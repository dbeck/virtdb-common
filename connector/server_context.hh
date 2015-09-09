#pragma once

#include <util/compare_messages.hh>
#include <string>
#include <set>
#include <memory>
#include <svc_config.pb.h>

namespace virtdb { namespace connector {

  class endpoint_client;
  
  class server_context
  {
  public:
    typedef std::set<std::string>                                              hosts_set;
    typedef std::set<interface::pb::EndpointData,util::compare_endpoint_data>  ep_data_set;

  private:
    std::string       service_name_;
    std::string       endpoint_svc_addr_;
    std::string       endpoint_hash_;
    uint64_t          ip_discovery_timeout_ms_;
    hosts_set         bind_also_to_;
    ep_data_set       endpoints_;
    
    server_context(const server_context &) = delete;
    server_context& operator=(const server_context &) = delete;
    
  public:
    typedef std::shared_ptr<server_context>  sptr;
    
    server_context();
    virtual ~server_context();
    
    // utilities
    bool keep_alive(endpoint_client & ep_cli);
    void increase_stat(const std::string & name,
                       double by_value=1.0);
    
    // setters
    void service_name(const std::string & name);
    void endpoint_svc_addr(const std::string & addr);
    void ip_discovery_timeout_ms(uint64_t val);
    void bind_also_to(const std::string & host);
    void add_endpoint(const interface::pb::EndpointData & ep);
    
    // getters
    std::string service_name() const;
    std::string endpoint_svc_addr() const;
    std::string endpoint_hash() const;
    uint64_t ip_discovery_timeout_ms() const;
    const hosts_set & bind_also_to() const;
    const ep_data_set & endpoints() const;
    
    static std::string hash_ep(const std::string & ep);
  };
  
}}
