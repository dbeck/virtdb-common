#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "column_dispatcher.hh"
#include <fault/injector.hh>
#include <sstream>

namespace virtdb { namespace dsproxy {
  
  void
  column_dispatcher::add_to_message_cache(const std::string & channel_id,
                                     const std::string & col_name,
                                     uint64_t block_id,
                                     data_sptr dta)
  {
    std::string message_key = channel_id + " " + col_name;
    message_id id = std::make_tuple(message_key, block_id);
    {
      std::unique_lock<std::mutex> l(message_cache_mtx_);
      message_cache_.insert(std::make_pair(id,dta));
    }
    
    // remove block in 3 mins
    timer_svc_.schedule(180000, [id,this]() {
      std::unique_lock<std::mutex> l(message_cache_mtx_);
      auto it = message_cache_.find(id);
      if( it != message_cache_.end() )
      {
        message_cache_.erase(id);
      }
      return false;
    });
  }
  
  bool
  column_dispatcher::resend_message(const std::string & query_id,
                               const std::string & segment_id,
                               const std::string & col_name,
                               uint64_t block_id)
  {
    std::string channel_id  = query_id + " " + segment_id;
    std::string message_key = channel_id + " " + col_name;
    
    message_id id = std::make_tuple(message_key, block_id);
    {
      std::unique_lock<std::mutex> l(message_cache_mtx_);
      auto it = message_cache_.find(id);
      if( it == message_cache_.end() )
      {
        LOG_TRACE("cannot resend chunk. not in the cache" <<
                 V_(query_id) <<
                 V_(segment_id) <<
                 V_(block_id));
        
        {
          std::unique_lock<std::mutex> l(block_id_mtx_);
          uint64_t min = 0;
          uint64_t max = 0;
          auto it = block_ids_.find(query_id);
          std::ostringstream os;
          if( it == block_ids_.end() )
          {
            os << "NOT_FOUND";
          }
          else
          {
            min = *(it->second.begin());
            max = *(it->second.rbegin());
            os << "missing:[";
            for( uint64_t i=0; i<max+1; ++i )
            {
              if( it->second.count(i) == 0 )
                os << i << ' ';
            }
            os << ']';
          }
          
          LOG_TRACE("missing block info" <<
                   V_(query_id) <<
                   V_(segment_id) <<
                   V_(min) <<
                   V_(max) <<
                   V_(os.str()));
        }
        return false;
      }
      else
      {
        UNLESS_INJECT_FAULT("omit-message", channel_id)
        {
          server_.publish(channel_id, it->second);
          LOG_TRACE("re-sending data block" <<
                   V_(query_id) <<
                   V_(segment_id) <<
                   V_(col_name) <<
                   V_(block_id) <<
                   V_(it->second->name()));
        }
        else
        {
          LOG_TRACE("not re-sending data block due to fault injection" <<
                   V_(query_id) <<
                   V_(segment_id) <<
                   V_(col_name) <<
                   V_(block_id) <<
                   V_(it->second->name()));
        }
        return true;
      }
    }
  }
  
  void
  column_dispatcher::subscribe_query(const std::string & query_id)
  {
    using namespace virtdb::interface;
    std::unique_lock<std::mutex> l(mtx_);

    if( client_sptr_ )
    {
      if( subscriptions_.count(query_id) == 0 )
      {
        client_sptr_->watch(query_id,[this](const std::string & provider_name,
                                            const std::string & channel,
                                            const std::string & subscription,
                                            std::shared_ptr<pb::Column> data)
                            {
                              handle_data(provider_name,
                                          channel,
                                          subscription,
                                          data);
                            });
        LOG_TRACE("subscribed to" << V_(query_id));
        subscriptions_.insert(query_id);
      }
      else
      {
        LOG_TRACE("already subscribed to" << V_(query_id));
      }
    }
    else
    {
      LOG_ERROR("cannot subscribe to" <<
                V_(query_id) <<
                "client is not ready yet");
    }
  }
  
  void
  column_dispatcher::unsubscribe_query(const std::string & query_id)
  {
    using namespace virtdb::interface;
    std::unique_lock<std::mutex> l(mtx_);
    
    if( client_sptr_ )
    {
      if( subscriptions_.count(query_id) > 0 )
      {
        client_sptr_->remove_watch(query_id);
        LOG_TRACE("unsubscribed from" << V_(query_id));
        subscriptions_.erase(query_id);
      }
    }
    else
    {
      LOG_ERROR("cannot unsubscribe from" <<
                V_(query_id) <<
                "client is not ready yet");
    }
  }
  
  bool
  column_dispatcher::reconnect()
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
  
  void
  column_dispatcher::watch_disconnect(on_disconnect m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_disconnect_ = m;
  }
  
  bool
  column_dispatcher::reconnect(const std::string & server)
  {
    using namespace virtdb::interface;
    
    {
      std::unique_lock<std::mutex> l(mtx_);
      subscriptions_.clear();
      client_sptr_.reset(new connector::column_client(client_ctx_,
                                                      *ep_client_, server));
    }
    
    bool ret = client_sptr_->wait_valid(util::SHORT_TIMEOUT_MS);
    
    if( ret )
    {
      LOG_TRACE("column client connected to:" << V_(server));
    }
    else
    {
      LOG_ERROR("column client connection to" <<
                V_(server) << "timed out in" <<
                V_(util::SHORT_TIMEOUT_MS));
    }
    return ret;
  }
  
  void
  column_dispatcher::add_to_backlog(const std::string & query_id,
                               data_sptr d)
  {
    {
      std::unique_lock<std::mutex> l(backlog_mtx_);
      auto it = backlog_.find(query_id);
      if( it == backlog_.end() )
      {
        auto rit = backlog_.insert(std::make_pair(query_id,
                                                  data_backlog()));
        it = rit.first;
      }
      it->second.push_back(d);
    }
    // schedule a safety check in 1h
    // when we cleanup the backlog
    if( d->seqno() == 0 )
    {
      std::string qid{query_id};
      timer_svc_.schedule(7200000, [qid,this]() {
        std::unique_lock<std::mutex> l(backlog_mtx_);
        auto it = backlog_.find(qid);
        if( it != backlog_.end() )
        {
          LOG_TRACE("removing query from the backlog that should have gone already" << V_(qid));
          backlog_.erase(qid);
        }
        return false;
      });
    }
  }
  
  void
  column_dispatcher::send_backlog(const std::string & query_id,
                             const std::string & segment_id)
  {
    std::unique_lock<std::mutex> l(backlog_mtx_);
    auto it = backlog_.find(query_id);
    if( it != backlog_.end() )
    {
      std::string channel_id = query_id + " " + segment_id;
      for( auto d : it->second )
      {
        server_.publish(channel_id, d);
      }
    }
  }
  
  void
  column_dispatcher::handle_data(const std::string & provider_name,
                            const std::string & channel,
                            const std::string & subscription,
                            std::shared_ptr<interface::pb::Column> data)
  {
    std::string new_channel_id{channel};
    other_channels others;
    
    try
    {
      handler_(provider_name,
               channel,
               subscription,
               data,
               new_channel_id,
               others);
    }
    catch( const std::exception & e )
    {
      LOG_ERROR("exception during channel id generation for segment host" <<
                E_(e));
    }
    catch( ... )
    {
      LOG_ERROR("unknown exception during channel id generation for segment host");
    }
    
    {
      // save sequence numbers for debug purposes
      std::unique_lock<std::mutex> l(block_id_mtx_);
      auto it = block_ids_.find(data->queryid());
      if( it == block_ids_.end() )
      {
        auto rit = block_ids_.insert(std::make_pair(data->queryid(),id_set()));
        it = rit.first;
      }
      it->second.insert(data->seqno());
    }
    
    // add message to cache before anything else
    add_to_message_cache(new_channel_id,
                         data->name(),
                         data->seqno(),
                         data);
    
    // send empty messages to others, if any
    {
      interface::pb::Column empty_data{*data};
      empty_data.clear_compresseddata();
      empty_data.clear_comptype();
      empty_data.clear_uncompressedsize();
      empty_data.clear_data();
      auto dta = empty_data.mutable_data();
      dta->set_type(data->data().type());

      // one copy to the backlog
      {
        std::shared_ptr<interface::pb::Column>
          empty_data_ptr{new interface::pb::Column{empty_data}};
        
        add_to_backlog(empty_data_ptr->queryid(),
                       empty_data_ptr);
      }
      
      for( auto const & other : others )
      {
        std::shared_ptr<interface::pb::Column>
          empty_data_ptr{new interface::pb::Column{empty_data}};
        
        // cache the empty messages too
        add_to_message_cache(other,
                             empty_data_ptr->name(),
                             empty_data_ptr->seqno(),
                             empty_data_ptr);
        
        UNLESS_INJECT_FAULT("omit-message", other)
        {
          LOG_TRACE("sending empty data to" <<
                    V_(other) <<
                    V_(channel) <<
                    V_(empty_data_ptr->queryid()) <<
                    V_(empty_data_ptr->seqno()) <<
                    V_(empty_data_ptr->name()) <<
                    V_(empty_data_ptr->endofdata()));

          server_.publish(other, empty_data_ptr);
        }
        else
        {
          LOG_TRACE("omit empty message due to fault injection" <<
                   V_(other) <<
                   V_(channel) <<
                   V_(empty_data_ptr->queryid()) <<
                   V_(empty_data_ptr->seqno()) <<
                   V_(empty_data_ptr->name()) );
        }
      }
    }
    
    UNLESS_INJECT_FAULT("omit-message", new_channel_id)
    {
      LOG_TRACE("publishing to" <<
                V_(new_channel_id) <<
                V_(channel) <<
                V_(data->queryid()) <<
                V_(data->seqno()) <<
                V_(data->name()) <<
                V_(data->endofdata()));

      server_.publish(new_channel_id, data);
    }
    else
    {
      LOG_TRACE("not publishing due to fault injection" <<
                V_(new_channel_id) <<
                V_(channel) <<
                V_(data->queryid()) <<
                V_(data->seqno()) <<
                V_(data->name()) <<
                V_(data->endofdata()));
    }
    
    if( data->endofdata() )
    {
      // schedule a backlog removal in 30 sec
      std::string qid{data->queryid()};
      timer_svc_.schedule(30000, [qid,this]() {
        std::unique_lock<std::mutex> l(backlog_mtx_);
        auto it = backlog_.find(qid);
        if( it != backlog_.end() )
        {
          LOG_TRACE("removing query from the backlog (1st pass)" << V_(qid));
          backlog_.erase(qid);
        }
        return false;
      });
      
      // schedule blockid removal in 3 mins
      timer_svc_.schedule(180000, [qid,this]() {
        std::unique_lock<std::mutex> l(block_id_mtx_);
        if( block_ids_.count(qid) > 0 )
          block_ids_.erase(qid);
        return false;
      });
    }
  }
  
  //const std::string & service_ep() const;
  //    const std::string & name() const;
  
  column_dispatcher::column_dispatcher(connector::server_context::sptr sr_ctx,
                                       connector::client_context::sptr cl_ctx,
                                       connector::config_client & cfg_clnt,
                                       on_data handler)
  : server_ctx_{sr_ctx},
    client_ctx_{cl_ctx},
    server_{sr_ctx, cfg_clnt},
    ep_client_{&(cfg_clnt.get_endpoint_client())},
    handler_{handler}
  {
    if( !handler )
    {
      THROW_("invalid handler");
    }
  }
  
  column_dispatcher::~column_dispatcher()
  {
  }
  
}}
