#pragma once

#include "endpoint_client.hh"
#include <data.pb.h>
#include <util/zmq_utils.hh>
#include <functional>
#include <vector>
#include <map>

namespace virtdb { namespace connector {
  
  class column_client final
  {
  public:
    struct column_location
    {
      std::string query_id_;
      std::string schema_;
      std::string table_;
      std::string column_;
      
      bool operator<(const column_location & rhs);
    };
    
    typedef std::function<void(const std::string & provider_name,
                               const column_location & location,
                               interface::pb::Column & data)>  column_monitor;
    
  private:
    typedef std::map<column_location, column_monitor>  monitor_map;
    typedef std::lock_guard<std::mutex>                lock;
    
    std::string                                  provider_;
    zmq::context_t                               zmqctx_;
    util::zmq_socket_wrapper                     column_sub_socket_;
    util::async_worker                           worker_;
    monitor_map                                  qualified_monitors_;
    monitor_map                                  wildcard_monitors_;
    mutable std::mutex                           sockets_mtx_;
    mutable std::mutex                           monitors_mtx_;
    
    bool worker_function();
    bool on_endpoint_data(const interface::pb::EndpointData & ep);
    
  public:
    column_client(endpoint_client & ep_client,
                  const std::string & provider_name);
    ~column_client();
    
    const std::string & provider_name() const;
    
    void watch(const column_location & location, column_monitor);
    void remove_watches();
    void remove_watch(const column_location & location);
    
  private:
    column_client() = delete;
    column_client(const column_client &) = delete;
    column_client & operator=(const column_client &) = delete;
  };
}}
