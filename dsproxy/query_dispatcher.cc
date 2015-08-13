#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <dsproxy/query_dispatcher.hh>
#include <connector/monitoring_client.hh>

namespace virtdb { namespace dsproxy {
  
  bool
  query_dispatcher::reconnect()
  {
    std::string server;
    {
      std::unique_lock<std::mutex> l(mtx_);
      if( client_sptr_ )
        server = client_sptr_->server();
    }
    if( server.empty() )
    {
      LOG_ERROR("cannot reconnect. empty server name");
      return false;
    }
    else
    {
      return reconnect(server);
    }    
  }
  
  bool
  query_dispatcher::reconnect(const std::string & server)
  {
    server_ctx_->increase_stat("Connect query proxy to server");

    {
      std::unique_lock<std::mutex> l(mtx_);
      client_sptr_.reset(new connector::query_client(client_ctx_,
                                                     *ep_client_, server));
    }
    
    bool ret = client_sptr_->wait_valid(util::SHORT_TIMEOUT_MS);
    
    if( ret )
    {
      LOG_TRACE("query client connected to:" << V_(server));
    }
    else
    {
      LOG_ERROR("query client connection to" <<
                V_(server) << "timed out in" <<
                V_(util::SHORT_TIMEOUT_MS));
    }
    return ret;
  }
  
  void
  query_dispatcher::watch_new_queries(on_new_query m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_new_query_ = m;
  }
  
  void
  query_dispatcher::watch_new_segments(on_new_segment m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_new_segment_ = m;
  }
  
  void
  query_dispatcher::watch_disconnect(on_disconnect m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_disconnect_ = m;
  }
  
  void
  query_dispatcher::watch_resend_chunk(on_resend_chunk m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_resend_chunk_ = m;
  }
  
  void
  query_dispatcher::remove_watch()
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_new_query empty;
    on_new_query_.swap(empty);
  }
  
  void
  query_dispatcher::handle_query(connector::query_server::query_sptr q)
  {
    if( !q )
    {
      LOG_ERROR("invalid query ptr");
      return;
    }
    
    server_ctx_->increase_stat("Incoming query");

    
    using namespace virtdb::connector;
    using namespace virtdb::interface;
    auto mon_cli = monitoring_client::global_instance();
    
    if( !q->has_queryid() ||
        !q->has_table() ||
        (q->fields_size() == 0 && q->querycontrol() != interface::pb::Query::STOP) )
    {
      server_ctx_->increase_stat("Invalid query");
      
      LOG_ERROR("cannot handle invalid query" <<
                V_(q->queryid()) <<
                V_(q->table()) <<
                V_(q->schema()) <<
                V_(q->fields_size()) <<
                V_(q->limit()) <<
                V_(q->filter_size()));
      
      if( mon_cli )
        mon_cli->report_bad_table_request(server_ctx_->service_name(),
                                          pb::MonitoringRequest::RequestError::INVALID_REQUEST,
                                          q->queryid(),
                                          q->table(),
                                          (q->has_schema()?q->schema().c_str():nullptr),
                                          "Missing fields from Query");
      return;
    }
    
    bool new_query     = false;
    bool cmd_query     = false;
    bool stop_query    = false;
    bool new_segment   = false;
    bool resend_chunk  = false;
    
    on_new_query      new_handler_copy;
    on_new_segment    segment_handler_copy;
    on_resend_chunk   resend_chunk_copy;
    {
      // create a query entry even if we won't be able to handle it
      std::unique_lock<std::mutex> l(mtx_);
      auto it = query_map_.find(q->queryid());
      if( it == query_map_.end() )
      {
        auto rit = query_map_.insert(std::make_pair(q->queryid(),string_set()));
        it = rit.first;
        new_query = true;
        new_handler_copy = on_new_query_;
      }
      else if( q->has_querycontrol() )
      {
        cmd_query = true;
        if( q->querycontrol() == interface::pb::Query::STOP )
        {
          server_ctx_->increase_stat("Query has STOP command (dispatcher)");
          stop_query = true;
        }
        else if( q->querycontrol() == interface::pb::Query::RESEND_CHUNK )
        {
          server_ctx_->increase_stat("Query has RESEND_CHUNK command (dispatcher)");
          if( q->has_segmentid() )
          {
            server_ctx_->increase_stat("Query has RESEND_CHUNK (segment) command (dispatcher)");
            resend_chunk = true;
            resend_chunk_copy = on_resend_chunk_;
          }
        }
        else if( q->querycontrol() == interface::pb::Query::RESEND_TABLE )
        {
          server_ctx_->increase_stat("Query has RESEND_TABLE command (dispatcher)");
        }
      }
      
      // save segment id if given
      if( q->has_segmentid() && it->second.count(q->segmentid()) == 0 )
      {
        it->second.insert(q->segmentid());
        LOG_TRACE("stored" << V_(q->segmentid()) << "for" << V_(q->queryid()));
        new_segment = true;
        segment_handler_copy = on_new_segment_;
      }
      
      // prepare stopped map too
      auto sit = stopped_map_.find(q->queryid());
      if( sit == stopped_map_.end() )
      {
        auto rit = stopped_map_.insert(std::make_pair(q->queryid(),string_set()));
        it = rit.first;
      }
    }
    
    client_sptr client_copy;
    {
      // save a copy of the client sptr, so that won't disappear
      // while sending the request i.e. reconnect()
      std::unique_lock<std::mutex> l(mtx_);
      if( !client_sptr_ )
      {
        LOG_ERROR("query client not yet initialized");
        if( mon_cli )
        {
          mon_cli->report_bad_table_request(server_ctx_->service_name(),
                                            pb::MonitoringRequest::RequestError::UPSTREAM_ERROR,
                                            q->queryid(),
                                            q->table(),
                                            (q->has_schema()?q->schema().c_str():nullptr),
                                            "Upstream provider not connected");
          mon_cli->report_state(server_ctx_->service_name(),
                                pb::MonitoringRequest::SetState::NOT_INITIALIZED);
        }
        return;
      }
      client_copy = client_sptr_;
    }
    
    // call new segment handler
    if( new_segment && segment_handler_copy )
    {
      try
      {
        segment_handler_copy(q->queryid(), q->segmentid());
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("exception caught" << E_(e));
      }
      catch (...)
      {
        LOG_ERROR("unknown exception caught");
      }
    }

    // call new query handler before we pass this query over
    // so channel subscription can go before data
    if( new_query && new_handler_copy )
    {
      try
      {
        new_handler_copy(q->queryid());
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("exception caught" << E_(e));
      }
      catch (...)
      {
        LOG_ERROR("unknown exception caught");
      }
    }
    
    if( !client_copy->wait_valid(util::SHORT_TIMEOUT_MS) )
    {
      LOG_ERROR("cannot serve request" <<
                M_(*q) <<
                "because query client connection to" <<
                V_(client_copy->server()) <<
                " timed out in" <<
                V_(util::SHORT_TIMEOUT_MS));
      return;
    }
    
    if( new_query || cmd_query )
    {
      if( stop_query )
      {
        bool skip_send = false;
        size_t stopped_count = 0;
        size_t segment_count = 0;
        {
          std::unique_lock<std::mutex> l(mtx_);
          stopped_map_[q->queryid()].insert(q->segmentid());
          stopped_count = stopped_map_[q->queryid()].size();
          segment_count = query_map_[q->queryid()].size();
          
          if( stopped_count == segment_count )
          {
            for( auto const & s : query_map_[q->queryid()] )
            {
              if( stopped_map_[q->queryid()].count(s) == 0 )
              {
                skip_send = true;
                break;
              }
            }
          }
          else
          {
            skip_send = true;
          }
        }
        
        if( !skip_send )
        {
          server_ctx_->increase_stat("Forwarding query");
          if( !client_copy->send_request(*q) )
          {
            if( mon_cli )
              mon_cli->report_bad_table_request(server_ctx_->service_name(),
                                                pb::MonitoringRequest::RequestError::UPSTREAM_ERROR,
                                                q->queryid(),
                                                q->table(),
                                                (q->has_schema()?q->schema().c_str():nullptr),
                                                "Failed to forward request");
          }
        }
        else
        {
          LOG_TRACE("delay sending STOP message until all segmenst say so" <<
                   V_(q->queryid()) <<
                   V_(q->segmentid()) <<
                   V_(stopped_count) <<
                   V_(segment_count) );
        }
      }
      else if( resend_chunk && resend_chunk_copy )
      {
        bool sent = false;
        
        try
        {
          std::set<uint64_t>     seqnos;
          std::set<std::string>  columns;
          
          for( auto sn : q->seqnos() )
          {
            seqnos.insert(sn);
          }
          
          for( auto nm : q->fields() )
          {
            columns.insert(nm);
          }
          
          sent = resend_chunk_copy(q->queryid(),
                                   q->segmentid(),
                                   columns,
                                   seqnos);
        }
        catch (const std::exception & e)
        {
          LOG_ERROR("exception caught" << E_(e));
        }
        catch (...)
        {
          LOG_ERROR("unknown exception caught");
        }
        
        // delegating the request to the data provider
        if( !sent )
        {
          server_ctx_->increase_stat("Forwarding query");
          if( !client_copy->send_request(*q) )
          {
            if( mon_cli )
              mon_cli->report_bad_table_request(server_ctx_->service_name(),
                                                pb::MonitoringRequest::RequestError::UPSTREAM_ERROR,
                                                q->queryid(),
                                                q->table(),
                                                (q->has_schema()?q->schema().c_str():nullptr),
                                                "Failed to forward request");
          }
          
        }
      }
      else
      {
        server_ctx_->increase_stat("Forwarding query");
        if( !client_copy->send_request(*q) )
        {
          if( mon_cli )
            mon_cli->report_bad_table_request(server_ctx_->service_name(),
                                              pb::MonitoringRequest::RequestError::UPSTREAM_ERROR,
                                              q->queryid(),
                                              q->table(),
                                              (q->has_schema()?q->schema().c_str():nullptr),
                                              "Failed to forward request");
        }

      }
    }
    else
    {
      server_ctx_->increase_stat("Query received multiple times");
      LOG_TRACE("skip sending the query multiple times" <<
                V_(q->queryid()));
    }
  }
  
  query_dispatcher::string_set
  query_dispatcher::query_segments(const std::string & query_id) const
  {
    std::unique_lock<std::mutex> l(mtx_);
    auto const it = query_map_.find(query_id);
    if( it != query_map_.end() )
      return it->second;
    else
      return string_set();
  }
  
  size_t
  query_dispatcher::query_count() const
  {
    std::unique_lock<std::mutex> l(mtx_);
    return query_map_.size();
  }
  
  void
  query_dispatcher::remove_segments(const std::string & query_id,
                               const string_set & segments)
  {
    std::unique_lock<std::mutex> l(mtx_);
    {
      auto it = query_map_.find(query_id);
      if( it != query_map_.end() )
      {
        for( const auto & s : segments )
        {
          it->second.erase(s);
        }
      }
    }
    {
      auto it = stopped_map_.find(query_id);
      if( it != stopped_map_.end() )
      {
        for( const auto & s : segments )
        {
          it->second.erase(s);
        }
      }
    }
  }
  
  void
  query_dispatcher::remove_query(const std::string & query_id)
  {
    std::unique_lock<std::mutex> l(mtx_);
    query_map_.erase(query_id);
    stopped_map_.erase(query_id);
  }
  
  query_dispatcher::query_dispatcher(connector::server_context::sptr sr_ctx,
                                     connector::client_context::sptr cl_ctx,
                                     connector::config_client & cfg_clnt,
                                     connector::user_manager_client::sptr umgr_cli,
                                     connector::srcsys_credential_client::sptr sscred_cli)
  : server_ctx_{sr_ctx},
    client_ctx_{cl_ctx},
    server_{sr_ctx, cfg_clnt, umgr_cli, sscred_cli},
    ep_client_(&(cfg_clnt.get_endpoint_client()))
  {
    server_.watch("", [&](const std::string & provider_name,
                          connector::query_server::query_sptr q,
                          connector::query_context::sptr qctx) {
      
      std::string segment_id;
      if( q->has_segmentid() )
        segment_id = q->segmentid();
      
      LOG_TRACE("query arrived" <<
               V_(q->queryid()) <<
               V_(q->table()) <<
               V_(q->fields_size()) <<
               V_(q->filter_size()) <<
               V_(segment_id));
   
      handle_query(std::move(q));
    });
  }
  
  query_dispatcher::~query_dispatcher()
  {
    server_.remove_watches();
  }
  
}}
