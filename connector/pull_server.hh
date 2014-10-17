#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <util/constants.hh>
#include <logger.hh>
#include "config_client.hh"
#include "server_base.hh"

namespace virtdb { namespace connector {
  
  template <typename ITEM>
  class pull_server : public server_base
  {
  public:
    typedef std::shared_ptr<ITEM>                pull_item_sptr;
    typedef std::function<void(pull_item_sptr)>  pull_handler;
    
  private:
    zmq::context_t                                                zmqctx_;
    util::zmq_socket_wrapper                                      socket_;
    util::async_worker                                            worker_;
    util::active_queue<pull_item_sptr,util::DEFAULT_TIMEOUT_MS>   queue_;
    pull_handler                                                  h_;
    
    bool worker_function()
    {
      if( !socket_.poll_in(util::DEFAULT_TIMEOUT_MS) )
        return true;
      
      // poll said we have data ...
      zmq::message_t message;
      
      if( !socket_.get().recv(&message) )
        return true;
      
      if( !message.data() || !message.size())
        return true;
      
      try
      {
        auto i = allocate_pull_item();
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
        std::string exception_text{e.what()};
        LOG_ERROR("couldn't parse message" << exception_text);
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
        release_pull_item(std::move(it));
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
    pull_server(config_client & cfg_client,
                pull_handler h)
    : server_base(cfg_client),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_PULL),
      worker_(std::bind(&pull_server::worker_function,
                        this)),
      queue_(1,std::bind(&pull_server::process_function,
                         this,
                         std::placeholders::_1)),
      h_(h)
    {
      socket_.batch_tcp_bind(hosts());
      worker_.start();
      
      // saving endpoint where we are bound to
      conn().set_type(interface::pb::ConnectionType::PUSH_PULL);
      for( auto const & ep : socket_.endpoints() )
        *(conn().add_address()) = ep;
    }

    virtual ~pull_server()
    {
      worker_.stop();
      queue_.stop();
    }
    
    pull_item_sptr allocate_pull_item()
    {
      return allocate_pull_item_impl();
    }
    
    void release_pull_item(pull_item_sptr && i)
    {
      release_pull_item_impl(std::move(i));
    }
    
  protected:
    virtual pull_item_sptr allocate_pull_item_impl()
    {
      // this is the place to recycle pointers if really wanted
      pull_item_sptr ret{new ITEM};
      return ret;
    }
    
    virtual void release_pull_item_impl(pull_item_sptr && i)
    {
      // make sure, refcount is 0 if recycled ...
    }
    
  private:
    pull_server() = delete;
    pull_server(const pull_server & other)  = delete;
    pull_server & operator=(const pull_server &) = delete;
  };
}}
