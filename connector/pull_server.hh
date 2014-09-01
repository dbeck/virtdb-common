#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include "config_client.hh"
#include "server_base.hh"

namespace virtdb { namespace connector {
  
  template <typename ITEM>
  class pull_server : public server_base
  {
  public:
    typedef std::shared_ptr<ITEM>           item_sptr;
    typedef std::function<void(item_sptr)>  handler;
    
  private:
    zmq::context_t                        zmqctx_;
    util::zmq_socket_wrapper              socket_;
    util::async_worker                    worker_;
    util::active_queue<item_sptr,15000>   queue_;
    handler                               h_;
    
    bool worker_function()
    {
      return false;
    }
    
    void process_function(item_sptr)
    {
    }
    
  public:
    pull_server(config_client & cfg_client,
                handler h)
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
    }

    virtual ~pull_server()
    {
      worker_.stop();
      queue_.stop();
    }
    
  private:
    pull_server() = delete;
    pull_server(const pull_server & other)  = delete;
    pull_server & operator=(const pull_server &) = delete;
  };
}}
