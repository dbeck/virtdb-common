#pragma once

#include <util/zmq_utils.hh>

namespace virtdb { namespace connector {
  
  class config_client;

  class server_base
  {
    util::zmq_socket_wrapper::host_set hosts_;
    
  protected:
    util::zmq_socket_wrapper::host_set & hosts();
    
  public:
    server_base(config_client & cfg_client);
    virtual const util::zmq_socket_wrapper::host_set & hosts() const;
    virtual ~server_base() {}
  };
}}
