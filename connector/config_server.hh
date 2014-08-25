#pragma once

#include <svc_config.pb.h>
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include "config_client.hh"
#include "endpoint_server.hh"
#include <map>

namespace virtdb { namespace connector {
  
  class config_server final
  {
    typedef std::map<std::string, interface::pb::Config> config_map;
    typedef std::lock_guard<std::mutex> lock;
    
    zmq::context_t               zmqctx_;
    util::zmq_socket_wrapper     cfg_rep_socket_;
    util::zmq_socket_wrapper     cfg_pub_socket_;
    util::async_worker           worker_;
    config_map                   configs_;
    std::mutex                   mtx_;

    bool worker_function();
    
  public:
    config_server(config_client & cfg_client,
                  endpoint_server & ep_server);
    ~config_server();
    
  private:
    config_server() = delete;
    config_server(const config_server &) = delete;
    config_server & operator=(const config_server &) = delete;
  };
}}
