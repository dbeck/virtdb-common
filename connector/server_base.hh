#pragma once

#include <connector/endpoint_client.hh>
#include <connector/server_context.hh>
#include <svc_config.pb.h>
#include <functional>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
  public:
    typedef std::function<const std::string & (server_context::sptr)> service_name_cb;
    
  private:
    server_context::sptr            context_;
    interface::pb::Connection       conn_;
    service_name_cb                 service_name_cb_;
    
  protected:
    interface::pb::Connection & conn();
    
  public:
    server_base(server_context::sptr ctx);
    
    virtual util::zmq_socket_wrapper::host_set
    hosts(endpoint_client & ep_client) const;
    
    virtual util::zmq_socket_wrapper::endpoint_set
      registered_endpoints(endpoint_client & ep_client,
                           interface::pb::ServiceType st,
                           interface::pb::ConnectionType ct) const;
    
    virtual const interface::pb::Connection & conn() const;
    virtual const std::string & name() const;
    virtual const std::string & ep_hash() const;
    virtual const std::string & service_name() const;
    virtual void override_service_name(service_name_cb);
    
    virtual ~server_base() {}

  private:
    server_base() = delete;
    server_base(const server_base &) = delete;
    server_base & operator=(const server_base &) = delete;
  };
}}
