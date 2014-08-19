#pragma once

#include <string>
#include <memory>

namespace virtdb { namespace connector {

  class endpoint_client final
  {
    endpoint_client() = delete;
    endpoint_client(const endpoint_client &) = delete;
    endpoint_client & operator=(const endpoint_client &) = delete;
    
  public:
    endpoint_client(const std::string & svc_config_ep,
                    const std::string & service_name);
    ~endpoint_client();
  };
  
}}
