#pragma once

#include <string>
#include <svc_config.pb.h>
#include <util/compare_messages.hh>
#include <util/async_worker.hh>
#include <util/active_queue.hh>
#include <util/zmq_utils.hh>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <mutex>

namespace virtdb { namespace connector {

  class endpoint_client
  {
  public:
    typedef std::function<void(const interface::pb::EndpointData &)> monitor;
    
  private:
    typedef std::set<interface::pb::EndpointData,util::compare_endpoint_data> ep_data_set;
    typedef std::vector<monitor> monitor_vector;
    typedef std::map<interface::pb::ServiceType, monitor_vector> monitor_map;
    typedef interface::pb::EndpointData ep_data_item;
    typedef util::active_queue<ep_data_item,util::DEFAULT_TIMEOUT_MS> notification_queue_t;

    std::string               service_ep_;
    std::string               name_;
    zmq::context_t            zmqctx_;
    util::zmq_socket_wrapper  ep_req_socket_;
    util::zmq_socket_wrapper  ep_sub_socket_;
    ep_data_set               endpoints_;
    monitor_map               monitors_;
    util::async_worker        worker_;
    notification_queue_t      queue_;
    mutable std::mutex        mtx_;
    
    void fire_monitor(monitor &, const interface::pb::EndpointData & ep);
    bool worker_function();
    void handle_endpoint_data(const interface::pb::EndpointData & ep);
    void async_handle_data(ep_data_item);
    
  public:
    endpoint_client(const std::string & svc_config_ep,
                    const std::string & service_name);
    virtual ~endpoint_client();
    
    void watch(interface::pb::ServiceType, monitor);
    void remove_watches(interface::pb::ServiceType);
    void remove_watches();
    void register_endpoint(const interface::pb::EndpointData &,
                           monitor m=[](const interface::pb::EndpointData &){return;});
    
    bool get(const std::string & ep_name,
             interface::pb::ServiceType st,
             interface::pb::EndpointData & result) const;
             
    const std::string & name() const;
    const std::string & service_ep() const;
    void cleanup();
    void rethrow_error();
    
    void wait_valid_req();
    void wait_valid_sub();
    
    bool wait_valid_req(uint64_t timeout_ms);
    bool wait_valid_sub(uint64_t timeout_ms);
    
  private:
    endpoint_client() = delete;
    endpoint_client(const endpoint_client &) = delete;
    endpoint_client & operator=(const endpoint_client &) = delete;
  };
  
}}
