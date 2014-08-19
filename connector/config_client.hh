#pragma once

#include "endpoint_client.hh"

namespace virtdb { namespace connector {
  
  class config_client final
  {
    config_client() = delete;
    config_client(const config_client &) = delete;
    config_client & operator=(const config_client &) = delete;
    
  public:
    config_client(endpoint_client & ep_client);
    ~config_client();
  };
}}
