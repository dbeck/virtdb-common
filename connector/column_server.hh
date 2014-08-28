#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class column_server final
  {
    // pub only
    
  public:
    column_server(config_client & cfg_client);
    ~column_server();
    
  private:
    column_server() = delete;
    column_server(const column_server &) = delete;
    column_server & operator=(const column_server &) = delete;
  };
}}
