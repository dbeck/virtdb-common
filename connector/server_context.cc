#include "server_context.hh"

namespace virtdb { namespace connector {
  
  server_context::server_context()
  : service_name_{"changeme"},
    ip_discovery_timeout_ms_{1000}
  {}
  
  server_context::~server_context() {}
  
  // setters
  void
  server_context::service_name(const std::string & name)
  {
    service_name_ = name;
  }
  
  void
  server_context::ip_discovery_timeout_ms(uint64_t val)
  {
    ip_discovery_timeout_ms_ = val;
  }
  
  // getters
  const std::string &
  server_context::service_name() const
  {
    return service_name_;
  }
  
  uint64_t
  server_context::ip_discovery_timeout_ms() const
  {
    return ip_discovery_timeout_ms_;
  }
  
}}
