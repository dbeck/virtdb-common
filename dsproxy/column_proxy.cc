#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "column_proxy.hh"
#include <fault/injector.hh>
#include <sstream>

namespace virtdb { namespace dsproxy {

  void
  column_proxy::subscribe_query(const std::string & query_id)
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
  column_proxy::unsubscribe_query(const std::string & query_id)
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
  column_proxy::reconnect()
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
  column_proxy::watch_disconnect(on_disconnect m)
  {
    std::unique_lock<std::mutex> l(mtx_);
    on_disconnect_ = m;
  }
  
  bool
  column_proxy::reconnect(const std::string & server)
  {
    using namespace virtdb::interface;
    
    {
      std::unique_lock<std::mutex> l(mtx_);
      subscriptions_.clear();
      
      if( client_sptr_ )
        client_sptr_->remove_watches();
      
      client_sptr_.reset(new connector::column_client(*ep_client_, server));
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
  column_proxy::handle_data(const std::string & provider_name,
                            const std::string & channel,
                            const std::string & subscription,
                            std::shared_ptr<interface::pb::Column> data)
  {
    try
    {
      handler_(provider_name,
               channel,
               subscription,
               data);
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
    
    UNLESS_INJECT_FAULT("omit-message", channel)
    {
      LOG_TRACE("publishing to" <<
                V_(channel) <<
                V_(data->queryid()) <<
                V_(data->seqno()) <<
                V_(data->name()) <<
                V_(data->endofdata()));

      server_.publish(channel, data);
    }
    else
    {
      LOG_TRACE("not publishing due to fault injection" <<
                V_(channel) <<
                V_(data->queryid()) <<
                V_(data->seqno()) <<
                V_(data->name()) <<
                V_(data->endofdata()));
    }
  }
    
  column_proxy::column_proxy(connector::config_client & cfg_clnt,
                             on_data handler)
  : server_(cfg_clnt),
    ep_client_(&(cfg_clnt.get_endpoint_client())), 
    handler_(handler)
  {
    if( !handler )
    {
      THROW_("invalid handler");
    }
  }
  
  column_proxy::~column_proxy()
  {
  }
  
}}
