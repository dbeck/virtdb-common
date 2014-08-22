#pragma once

#include "endpoint_client.hh"

namespace virtdb { namespace connector {
  
  class config_client final
  {
    endpoint_client * ep_client_;
    
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
