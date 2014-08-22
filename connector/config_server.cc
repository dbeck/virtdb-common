#include "config_server.hh"
#include <logger.hh>

namespace virtdb { namespace connector {
  
  /*
   zmq::context_t       zmqctx_;
   zmq::socket_t        cfg_pull_socket_;
   zmq::socket_t        cfg_pub_socket_;
   util::async_worker   worker_;
   */
  
  config_server::config_server(config_client & cfg_client)
  : zmqctx_(1),
    cfg_pull_socket_(zmqctx_, ZMQ_PULL),
    cfg_pub_socket_(zmqctx_, ZMQ_PUB),
    worker_(std::bind(&config_server::worker_function,this))
  {
    // TODO : initialize
    worker_.start();
  }
  
  config_server::~config_server()
  {
    worker_.stop();
  }
  
  bool
  config_server::worker_function()
  {
    LOG_ERROR("TODO : implement me");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
  }
}}
