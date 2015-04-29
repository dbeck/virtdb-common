

#include "server_context.hh"
#include <util/hex_util.hh>
#include <xxhash.h>

namespace virtdb { namespace connector {
  
  server_context::server_context()
  : service_name_{"changeme"},
    endpoint_svc_addr_{"changeme"},
    ip_discovery_timeout_ms_{1000}
  {
  }
  
  server_context::~server_context() {}
  
  void
  server_context::service_name(const std::string & name)
  {
    service_name_ = name;
  }
  
  void
  server_context::endpoint_svc_addr(const std::string & addr)
  {
    endpoint_svc_addr_ = addr;
    
    // generate a hash on the endpoint service
    endpoint_hash_ = hash_ep(addr);
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
  
  const std::string &
  server_context::endpoint_svc_addr() const
  {
    return endpoint_svc_addr_;
  }
  
  const std::string &
  server_context::endpoint_hash() const
  {
    return endpoint_hash_;
  }

  uint64_t
  server_context::ip_discovery_timeout_ms() const
  {
    return ip_discovery_timeout_ms_;
  }
  
  void
  server_context::bind_also_to(const std::string & host)
  {
    bind_also_to_.insert(host);
  }
  
  const server_context::hosts_set &
  server_context::bind_also_to() const
  {
    return bind_also_to_;
  }
  
  std::string
  server_context::hash_ep(const std::string & ep_string)
  {
    std::string ret;
    util::hex_util(XXH64(ep_string.c_str(), ep_string.size(), 0), ret);
    return ret;
  }
  
}}
