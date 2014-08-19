#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class log_record_server final
  {
    log_record_server() = delete;
    log_record_server(const log_record_server &) = delete;
    log_record_server & operator=(const log_record_server &) = delete;
  public:
    log_record_server(config_client & cfg_client);
    ~log_record_server();
  };
}}
