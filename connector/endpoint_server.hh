#pragma once

#include <string>
#include <svc_config.pb.h>
#include <util/compare_messages.hh>
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include <util/timer_service.hh>
#include <connector/server_context.hh>
#include <connector/ip_discovery_server.hh>
#include <set>
#include <map>

namespace virtdb { namespace connector {
  
  class endpoint_server
  {
    typedef std::set<interface::pb::EndpointData,util::compare_endpoint_data>  ep_data_set;
    typedef std::map<std::string, uint64_t> keep_alive_map;
    
    server_context::sptr        context_;
    std::string                 local_ep_;
    std::string                 global_ep_;
    ep_data_set                 endpoints_;
    zmq::context_t              zmqctx_;
    util::zmq_socket_wrapper    ep_rep_socket_;
    util::zmq_socket_wrapper    ep_pub_socket_;
    util::async_worker          worker_;
    ip_discovery_server         discovery_;
    util::timer_service         timer_svc_;
    keep_alive_map              keep_alive_;
    std::mutex                  mtx_;
    
    bool worker_function();
    void add_endpoint_data(const interface::pb::EndpointData & dta);
    void publish_endpoint(const interface::pb::EndpointData & dta);
    
  public:
    endpoint_server(server_context::sptr ctx);
    
    const std::string & local_ep() const;
    const std::string & global_ep() const;
    const std::string & name() const;
    
    void reload_from(const std::string & path);
    void save_to(const std::string & path);
    
    virtual ~endpoint_server();
    void cleanup();
    void rethrow_error();
    
  private:
    endpoint_server() = delete;
    endpoint_server(const endpoint_server &) = delete;
    endpoint_server & operator=(const endpoint_server &) = delete;
  };
}}
