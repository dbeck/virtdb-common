#pragma once

#include <string>
#include <zmq.hpp>
#include <svc_config.pb.h>
#include <util/compare_messages.hh>
#include <util/barrier.hh>
#include <util/async_worker.hh>
#include <map>
#include <vector>
#include <set>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace virtdb { namespace connector {

  class endpoint_client final
  {
  public:
    typedef std::function<bool(const interface::pb::EndpointData &)> monitor;
    
  private:
    typedef std::set<interface::pb::EndpointData,util::compare_endpoint_data> ep_data_set;
    typedef std::vector<monitor> monitor_vector;
    typedef std::map<interface::pb::ServiceType, monitor_vector> monitor_map;

    std::string        name_;
    zmq::context_t     zmqctx_;
    zmq::socket_t      ep_req_socket_;
    zmq::socket_t      ep_sub_socket_;
    ep_data_set        endpoints_;
    monitor_map        monitors_;
    util::async_worker worker_;
    std::mutex         mtx_;
    
    bool fire_monitor(monitor &, const interface::pb::EndpointData & ep);
    bool worker_function();
    void handle_endpoint_data(const interface::pb::EndpointData & ep);
    
  public:
    endpoint_client(const std::string & svc_config_ep,
                    const std::string & service_name);
    ~endpoint_client();
    
    void watch(interface::pb::ServiceType, monitor);
    const std::string & name() const;
    
  private:
    endpoint_client() = delete;
    endpoint_client(const endpoint_client &) = delete;
    endpoint_client & operator=(const endpoint_client &) = delete;
  };
  
}}
