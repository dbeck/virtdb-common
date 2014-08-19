#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class query_server final
  {
    query_server() = delete;
    query_server(const query_server &) = delete;
    query_server & operator=(const query_server &) = delete;
  public:
    query_server(config_client & cfg_client);
    ~query_server();
  };
}}
