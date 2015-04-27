#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace virtdb { namespace connector {
  
  class server_context
  {
    std::string  service_name_;
    uint64_t     ip_discovery_timeout_ms_;
    
    server_context(const server_context &) = delete;
    server_context& operator=(const server_context &) = delete;
    
  public:
    typedef std::shared_ptr<server_context> sptr;
    
    server_context();
    virtual ~server_context();
    
    // setters
    void service_name(const std::string & name);
    void ip_discovery_timeout_ms(uint64_t val);
    
    // getters
    const std::string & service_name() const;
    uint64_t ip_discovery_timeout_ms() const;
  };
  
}}
