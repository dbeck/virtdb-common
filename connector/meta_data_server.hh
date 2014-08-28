#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class meta_data_server final
  {
    // rep, pub
    
  public:
    meta_data_server(config_client & cfg_client);
    ~meta_data_server();
    
  private:
    meta_data_server() = delete;
    meta_data_server(const meta_data_server &) = delete;
    meta_data_server & operator=(const meta_data_server &) = delete;
  };
  
}}
