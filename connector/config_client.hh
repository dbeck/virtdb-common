#pragma once

#include "endpoint_client.hh"

namespace virtdb { namespace connector {
  
  class config_client final
  {
    endpoint_client * ep_client_;
    zmq::context_t    zmqctx_;
    zmq::socket_t        ep_req_socket_;
    zmq::socket_t        ep_sub_socket_;
    
  public:
    config_client(endpoint_client & ep_client);
    ~config_client();
    
    endpoint_client & get_endpoint_client();
    
  private:
    config_client() = delete;
    config_client(const config_client &) = delete;
    config_client & operator=(const config_client &) = delete;
  };
}}
