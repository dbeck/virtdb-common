#include "endpoint_server.hh"

namespace virtdb { namespace connector {
  
  endpoint_server::endpoint_server(const std::string & svc_endpoint) {}
  endpoint_server::~endpoint_server() {}
  
  const std::string &
  endpoint_server::local() const
  {
    static const std::string ret{"local"};
    return ret;
  }
}}
