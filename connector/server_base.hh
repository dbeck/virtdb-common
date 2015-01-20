#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>
#include <connector/endpoint_client.hh>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
    std::string                             name_;
    util::zmq_socket_wrapper::host_set      hosts_;
    interface::pb::Connection               conn_;
    
  protected:
    virtual interface::pb::Connection & conn();
    
  public:
    server_base(config_client & cfg_client);
    virtual const util::zmq_socket_wrapper::host_set & hosts() const;
    virtual const util::zmq_socket_wrapper::host_set & additional_hosts() const;
    
    virtual util::zmq_socket_wrapper::endpoint_set
      registered_endpoints(endpoint_client & ep_client,
                           interface::pb::ServiceType st,
                           interface::pb::ConnectionType ct) const;
    
    virtual const interface::pb::Connection & conn() const;
    virtual const std::string & name() const;
    virtual ~server_base() {}
    
  private:
    server_base() = delete;
    server_base(const server_base &) = delete;
    server_base & operator=(const server_base &) = delete;
  };
}}
