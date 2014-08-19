#pragma once

#include <string>

namespace virtdb { namespace connector {
  
  class endpoint_server final
  {
    endpoint_server() = delete;
    endpoint_server(const endpoint_server &) = delete;
    endpoint_server & operator=(const endpoint_server &) = delete;
  public:
    endpoint_server(const std::string & svc_endpoint);
    ~endpoint_server();
    const std::string & local() const;
  };
}}
