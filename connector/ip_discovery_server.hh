#pragma once

#include <util/barrier.hh>
#include <util/async_worker.hh>
#include <utility>
#include <set>
#include <string>
#include <thread>
#include <atomic>

namespace virtdb { namespace connector {
  
  class ip_discovery_server final
  {
  public:
    typedef std::set<std::string> endpoint_set;
    ip_discovery_server(size_t n_retries_on_exception=10,
                        bool die_on_exception=true);
    ~ip_discovery_server();
    
    void cleanup();
    void rethrow_error();
    
    // this function tells where the server bound to
    // returns a vector of strings:
    //   123.123.123.123:65432  for ipv4
    //   [1::2::3::4::]:65432   for ipv6
    const endpoint_set &
    endpoints() const;
    
  private:
    bool handle_requests();
    
    endpoint_set         endpoints_;
    util::async_worker   worker_;
    int                  fd_ipv4_;
    int                  fd_ipv6_;
    
    ip_discovery_server(const ip_discovery_server &) = delete;
    ip_discovery_server & operator=(const ip_discovery_server &) = delete;
  };
}}
