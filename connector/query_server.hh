#pragma once

#include "config_client.hh"
#include "column_location.hh"
#include <util/active_queue.hh>
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include <functional>
#include <data.pb.h>
#include <memory>

namespace virtdb { namespace connector {
  
  class query_server final
  {
  public:
    typedef std::shared_ptr<interface::pb::Query> query_sptr;
    
    typedef std::function<void(const std::string & provider_name,
                               const column_location & location,
                               query_sptr data)> query_monitor;
  private:
    
    typedef std::map<column_location, query_monitor>   monitor_map;
    typedef std::lock_guard<std::mutex>                lock;
    
    zmq::context_t                          zmqctx_;
    util::zmq_socket_wrapper                query_pull_socket_;
    util::async_worker                      worker_;
    util::active_queue<query_sptr,15000>    query_monitor_queue_;
    monitor_map                             qualified_monitors_;
    monitor_map                             wildcard_monitors_;
    mutable std::mutex                      sockets_mtx_;
    mutable std::mutex                      monitors_mtx_;
    
    bool worker_function();
    void handler_function(query_sptr);
    
  public:
    query_server(config_client & cfg_client);
    ~query_server();
    
    void watch(const column_location & location, query_monitor);
    void remove_watches();
    void remove_watch(const column_location & location);

  private:
    query_server() = delete;
    query_server(const query_server &) = delete;
    query_server & operator=(const query_server &) = delete;
  };
}}
