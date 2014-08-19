#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class config_server final
  {
    config_server() = delete;
    config_server(const config_server &) = delete;
    config_server & operator=(const config_server &) = delete;
  public:
    config_server(config_client & cfg_client);
    ~config_server();
  };
}}
