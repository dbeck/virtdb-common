#pragma once

#include <util/zmq_utils.hh>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
    std::string                          name_;
    util::zmq_socket_wrapper::host_set   hosts_;
    
  public:
    server_base(config_client & cfg_client);
    virtual const util::zmq_socket_wrapper::host_set & hosts() const;
    virtual const std::string & name() const;
    virtual ~server_base() {}
  };
}}
