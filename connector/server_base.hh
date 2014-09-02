#pragma once

#include <svc_config.pb.h>
#include <util/zmq_utils.hh>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
    std::string                          name_;
    util::zmq_socket_wrapper::host_set   hosts_;
    interface::pb::Connection            conn_;
    
  protected:
    virtual interface::pb::Connection & conn();
    
  public:
    server_base(config_client & cfg_client);
    virtual const util::zmq_socket_wrapper::host_set & hosts() const;
    virtual const util::zmq_socket_wrapper::host_set & additional_hosts() const;
    virtual const interface::pb::Connection & conn() const;
    virtual const std::string & name() const;
    virtual ~server_base() {}
    
    static bool poll_socket(util::zmq_socket_wrapper & s,
                            unsigned long timeout_ms);
    
  private:
    server_base() = delete;
    server_base(const server_base &) = delete;
    server_base & operator=(const server_base &) = delete;
  };
}}
