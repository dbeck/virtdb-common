#pragma once

#include <string>
#include <set>
#include <memory>

namespace virtdb { namespace connector {
  
  class server_context
  {
  public:
    typedef std::set<std::string> hosts_set;
    
  private:
    std::string       service_name_;
    std::string       endpoint_svc_addr_;
    std::string       endpoint_hash_;
    uint64_t          ip_discovery_timeout_ms_;
    hosts_set         bind_also_to_;
    
    server_context(const server_context &) = delete;
    server_context& operator=(const server_context &) = delete;
    
  public:
    typedef std::shared_ptr<server_context>  sptr;
    
    server_context();
    virtual ~server_context();
    
    // setters
    void service_name(const std::string & name);
    void endpoint_svc_addr(const std::string & addr);
    void ip_discovery_timeout_ms(uint64_t val);
    void bind_also_to(const std::string & host);
    
    // getters
    const std::string & service_name() const;
    const std::string & endpoint_svc_addr() const;
    const std::string & endpoint_hash() const;
    uint64_t ip_discovery_timeout_ms() const;
    const hosts_set & bind_also_to() const;
    
    static std::string hash_ep(const std::string & ep);
  };
  
}}
