#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <dsproxy/query_proxy.hh>
#include <connector/monitoring_client.hh>

namespace virtdb { namespace dsproxy {
  
  bool
  query_proxy::reconnect()
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
  query_proxy::reconnect(const std::string & server)
  {
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
  query_proxy::watch_new_queries(on_new_query m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_new_query_ = m;
  }
  
  void
  query_proxy::watch_disconnect(on_disconnect m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_disconnect_ = m;
  }
  
  void
  query_proxy::watch_resend_chunk(on_resend_chunk m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_resend_chunk_ = m;
  }
  
  void
  query_proxy::remove_watch()
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_new_query empty;
    on_new_query_.swap(empty);
  }
  
  void
  query_proxy::handle_query(connector::query_server::query_sptr q)
  {
    if( !q )
    {
      LOG_ERROR("invalid query ptr");
      return;
    }
    
    using namespace virtdb::connector;
    using namespace virtdb::interface;
    auto mon_cli = monitoring_client::global_instance();
    
    if( !q->has_queryid() ||
        !q->has_table() ||
        !q->fields_size() )
    {
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
    bool resend_chunk  = false;
    
    on_new_query      new_handler_copy;
    on_resend_chunk   resend_chunk_copy;
    {
      // create a query entry even if we won't be able to handle it
      std::unique_lock<std::mutex> l(mtx_);
      // TODO : FIXME
      new_handler_copy = on_new_query_;
      new_query = true;
      
      if( q->has_querycontrol() )
      {
        cmd_query = true;
        if( q->querycontrol() == interface::pb::Query::STOP )
        {
          stop_query = true;
        }
        else if( q->querycontrol() == interface::pb::Query::RESEND_CHUNK )
        {
          resend_chunk = true;
          resend_chunk_copy = on_resend_chunk_;
        }
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
    
    // call new query handler before we pass this query over
    // so channel subscription can go before data
    action query_action = forward_query;
    if( new_query && new_handler_copy )
    {
      try
      {
        query_action = new_handler_copy(q->queryid(), q);
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
        client_copy->send_request(*q);
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
          
          for( auto fl : q->fields() )
          {
            columns.insert(fl.name());
          }
          
          sent = resend_chunk_copy(q->queryid(),
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
        if( sent )
        {
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
      else if( query_action == dont_forward )
      {
        LOG_TRACE("not forwarding the query to" <<
                  V_(client_copy->server()) <<
                  V_(q->queryid()) <<
                  V_(q->schema()) <<
                  V_(q->table()) <<
                  V_(q->has_querycontrol()) <<
                  V_((int)q->querycontrol()) <<
                  V_(q->seqnos_size()));
      }
      else
      {
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
      LOG_TRACE("skip sending the query multiple times" <<
                V_(q->queryid()));
    }
  }
    
  query_proxy::query_proxy(connector::server_context::sptr sr_ctx,
                           connector::client_context::sptr cl_ctx,
                           connector::config_client & cfg_clnt)
  : server_ctx_{sr_ctx},
    client_ctx_{cl_ctx},
    server_{sr_ctx, cfg_clnt},
    ep_client_(&(cfg_clnt.get_endpoint_client()))
  {
    server_.watch("", [&](const std::string & provider_name,
                          connector::query_server::query_sptr q) {
      std::string segment_id;
      if( q->has_segmentid() )
        segment_id = q->segmentid();
      
      LOG_TRACE("query arrived" <<
                V_(q->queryid()) <<
                V_(q->table()) <<
                V_(q->fields_size()) <<
                V_(q->filter_size()) <<
                V_(segment_id) <<
                V_(q->has_querycontrol()) <<
                V_((int)q->querycontrol()) <<
                V_(q->seqnos_size()));
   
      handle_query(std::move(q));
    });
  }
  
  query_proxy::~query_proxy()
  {
    server_.remove_watches();
  }
  
}}
