#pragma once

#include "endpoint_client.hh"
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include "req_client.hh"
#include "service_type_map.hh"
#include <functional>
#include <vector>
#include <map>

namespace virtdb { namespace connector {
  
  class config_client final :
      public req_client<interface::pb::Config, interface::pb::Config>
  {
    typedef req_client<interface::pb::Config, interface::pb::Config> req_base_type;
    
  public:
    typedef std::function<bool(const interface::pb::Config &)> cfg_monitor;
    
  private:
    typedef std::vector<cfg_monitor>                 monitor_vector;
    typedef std::map<std::string, monitor_vector>    monitor_map;
    typedef std::lock_guard<std::mutex>              lock;

    endpoint_client *             ep_client_;
    zmq::context_t                zmqctx_;
    util::zmq_socket_wrapper      cfg_sub_socket_;
    util::async_worker            worker_;
    monitor_map                   monitors_;
    mutable std::mutex            sockets_mtx_;
    mutable std::mutex            monitors_mtx_;
    
    bool worker_function();
    bool on_endpoint_data(const interface::pb::EndpointData & ep);

  public:
    config_client(endpoint_client & ep_client,
                  const std::string & server_name);
    virtual ~config_client();
    
    endpoint_client & get_endpoint_client();
    
    void watch(const std::string & name, cfg_monitor);
    void remove_watches();
    void remove_watches(const std::string & name);
  };
}}
