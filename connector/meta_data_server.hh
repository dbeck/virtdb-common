#pragma once

#include "config_client.hh"

namespace virtdb { namespace connector {
  
  class meta_data_server final
  {
    // rep, pub
    
    /* TODO
    
    zmq::context_t                zmqctx_;
    util::zmq_socket_wrapper      diag_rep_socket_;
    util::zmq_socket_wrapper      diag_pub_socket_;
    util::async_worker            worker_;
    mutable std::mutex            mtx_;

    
    bool worker_function();
    bool on_endpoint_data(const interface::pb::EndpointData & ep);
     */
    
  public:
    meta_data_server(config_client & cfg_client);
    ~meta_data_server();
    
  private:
    meta_data_server() = delete;
    meta_data_server(const meta_data_server &) = delete;
    meta_data_server & operator=(const meta_data_server &) = delete;
  };
  
}}
