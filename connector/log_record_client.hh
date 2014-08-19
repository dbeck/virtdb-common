#pragma once

#include "endpoint_client.hh"

namespace virtdb { namespace connector {
  
  class log_record_client final
  {
    log_record_client() = delete;
    log_record_client(const log_record_client &) = delete;
    log_record_client & operator=(const log_record_client &) = delete;
    
  public:
    log_record_client(endpoint_client & ep_client);
    ~log_record_client();
  };
}}
