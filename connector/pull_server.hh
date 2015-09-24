#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <util/constants.hh>
#include <logger.hh>
#include <connector/config_client.hh>
#include <connector/server_base.hh>

namespace virtdb { namespace connector {
  
  template <typename ITEM>
  class pull_server : public server_base
  {
  public:
    typedef ITEM                                 pull_item;
    typedef std::shared_ptr<pull_item>           pull_item_sptr;
    typedef std::function<void(pull_item_sptr)>  pull_handler;
    
  private:
    zmq::context_t                                                zmqctx_;
    util::zmq_socket_wrapper                                      socket_;
    util::async_worker                                            worker_;
    util::active_queue<pull_item_sptr,util::DEFAULT_TIMEOUT_MS>   queue_;
    pull_handler                                                  h_;
    
    bool worker_function()
    {
      if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS,
                           util::SHORT_TIMEOUT_MS) )
        return true;
      
      // poll said we have data ...
      zmq::message_t message(0);
      
      if( !socket_.get().recv(&message) )
        return true;
      
      if( !message.data() || !message.size())
        return true;
      
      try
      {
        auto i = pull_item_sptr{new pull_item};
        if( i->ParseFromArray(message.data(), message.size()) )
        {
          queue_.push(std::move(i));
        }
        else
        {
          LOG_ERROR("failed to parse message" << V_(i->GetTypeName()));
        }
      }
      catch (const std::exception & e)
      {
        LOG_ERROR("couldn't parse message" << E_(e));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception");
      }
      
      return true;
    }
    
    void process_function(pull_item_sptr it)
    {
      try
      {
        if( h_ )
        {
          h_(it);
        }
      }
      catch(const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown exception caught");
      }
    }
    
  public:
    pull_server(server_context::sptr ctx,
                config_client & cfg_client,
                pull_handler h,
                interface::pb::ServiceType st)
    : server_base{ctx},
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_PULL),
      worker_(std::bind(&pull_server::worker_function,
                        this),
              /* catch exception and ignore request.
                 users expected to check exceptions by calling
                 rethrow_error(). */
              10, false),
      queue_(1,std::bind(&pull_server::process_function,
                         this,
                         std::placeholders::_1)),
      h_(h)
    {
      // save endpoint_client ref
      endpoint_client & ep_client = cfg_client.get_endpoint_client();
      
      // save endpoint_set ref
      util::zmq_socket_wrapper::endpoint_set
      ep_set{registered_endpoints(ep_client,
                                  st,
                                  interface::pb::ConnectionType::PUSH_PULL)};
      
      if( !socket_.batch_ep_rebind(ep_set, true) )
      {
        socket_.batch_tcp_bind(hosts(ep_client));
      }
      else
      {
        LOG_TRACE("rebound to previous endpoint addresses");
      }
      
      worker_.start();
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::PUSH_PULL);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }

    virtual ~pull_server()
    {
      socket_.stop();
      worker_.stop();
      queue_.stop();
    }
    
    virtual void cleanup()
    {
      socket_.disconnect_all();
      socket_.stop();
      worker_.stop();
      queue_.stop();
    }
    
    virtual void rethrow_error()
    {
      worker_.rethrow_error();
    }
        
  private:
    pull_server() = delete;
    pull_server(const pull_server & other)  = delete;
    pull_server & operator=(const pull_server &) = delete;
  };
}}
