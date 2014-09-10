#pragma once

#include <string>
#include <svc_config.pb.h>
#include <util/compare_messages.hh>
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include "ip_discovery_server.hh"
#include <set>

namespace virtdb { namespace connector {
  
  class endpoint_server final
  {
    typedef std::set<interface::pb::EndpointData,util::compare_endpoint_data> ep_data_set;
    
    std::string                 name_;
    std::string                 local_ep_;
    std::string                 global_ep_;
    ep_data_set                 endpoints_;
    zmq::context_t              zmqctx_;
    util::zmq_socket_wrapper    ep_rep_socket_;
    util::zmq_socket_wrapper    ep_pub_socket_;
    util::async_worker          worker_;
    ip_discovery_server         discovery_;
    std::mutex                  mtx_;
    
    bool worker_function();
    
  public:
    endpoint_server(const std::string & svc_endpoint,
                    const std::string & service_name);
    
    const std::string & local_ep() const;
    const std::string & global_ep() const;
    const std::string & name() const;
    
    ~endpoint_server();
    
  private:
    endpoint_server() = delete;
    endpoint_server(const endpoint_server &) = delete;
    endpoint_server & operator=(const endpoint_server &) = delete;
  };
}}
