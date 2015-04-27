#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <connector/endpoint_client.hh>

#include <connector/server_context.hh>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
    server_context::sptr                    context_;
    std::string                             name_;
    interface::pb::Connection               conn_;
    std::string                             ep_hash_;
    
  protected:
    virtual interface::pb::Connection & conn();
    
    // config values, override if needed
    virtual uint64_t ip_discovery_timeout_ms() const
    {
      return 1000;
    };
    
  public:
    server_base(server_context::sptr ctx,
                config_client & cfg_client);
    virtual util::zmq_socket_wrapper::host_set hosts(endpoint_client & ep_client) const;
    virtual const util::zmq_socket_wrapper::host_set & additional_hosts() const;
    
    virtual util::zmq_socket_wrapper::endpoint_set
      registered_endpoints(endpoint_client & ep_client,
                           interface::pb::ServiceType st,
                           interface::pb::ConnectionType ct) const;
    
    virtual const interface::pb::Connection & conn() const;
    virtual const std::string & name() const;
    virtual const std::string & ep_hash() const;
    virtual ~server_base() {}
    
    static std::string hash_ep(const std::string & ep);
    
  private:
    server_base() = delete;
    server_base(const server_base &) = delete;
    server_base & operator=(const server_base &) = delete;
  };
}}
