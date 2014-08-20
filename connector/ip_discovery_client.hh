#pragma once

#include <vector>
#include <string>

namespace virtdb { namespace connector {
  
  class ip_discovery_client final
  {
  public:
    typedef std::vector<std::string> endpoint_vector;
    
    // returns our IP as the discovery server sees it, so we
    // will know if there is NAT between us and the discovery server.
    // possible formats are:
    //   123.123.123.123:65432  for ipv4
    //   [1::2::3::4::]:65432   for ipv6
    static std::string
    get_ip(const endpoint_vector & srv_endpoints);
  };
}}
