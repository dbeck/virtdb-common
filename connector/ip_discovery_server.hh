#pragma once

#include <util/barrier.hh>
#include <utility>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace virtdb { namespace connector {
  
  class ip_discovery_server final
  {
  public:
    typedef std::vector<std::string> endpoint_vector;
    ip_discovery_server();
    ~ip_discovery_server();
    
    // this function tells where the server bound to
    // returns a vector of strings:
    //   123.123.123.123:65432  for ipv4
    //   [1::2::3::4::]:65432   for ipv6
    const endpoint_vector &
    endpoints() const;
    
  private:
    void handle_requests();
    
    endpoint_vector    endpoints_;
    util::barrier      barrier_;
    std::atomic<bool>  stop_me_;
    std::thread        worker_;
    int                fd_ipv4_;
    int                fd_ipv6_;
    
    ip_discovery_server(const ip_discovery_server &) = delete;
    ip_discovery_server & operator=(const ip_discovery_server &) = delete;
  };
}}
