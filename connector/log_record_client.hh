#pragma once

#include "endpoint_client.hh"
#include <util/async_worker.hh>
#include <zmq.hpp>
#include <memory>
#include <logger.hh>
#include <map>
#include <functional>
#include <mutex>

namespace virtdb { namespace connector {
  
  class log_record_client final
  {
  public:
    typedef std::function<void(const std::string & name,
                               interface::pb::LogRecord & rec)>  log_monitor;
    
  private:
    log_record_client() = delete;
    log_record_client(const log_record_client &) = delete;
    log_record_client & operator=(const log_record_client &) = delete;
    
    
    typedef std::map<std::string, log_monitor>   monitor_map;
    typedef std::lock_guard<std::mutex>          lock;
    
    zmq::context_t                               zmqctx_;
    std::shared_ptr<zmq::socket_t>               logger_push_socket_sptr_;
    std::shared_ptr<zmq::socket_t>               logger_sub_socket_sptr_;
    std::shared_ptr<zmq::socket_t>               logger_req_socket_sptr_;
    std::shared_ptr<virtdb::logger::log_sink>    log_sink_sptr_;
    // TODO : refactor this {ep,socket} binding into util::zmq_utils
    std::string                                  push_logger_ep_;
    std::string                                  sub_logger_ep_;
    std::string                                  req_logger_ep_;
    util::async_worker                           worker_;
    monitor_map                                  monitors_;
    mutable std::mutex                           sockets_mtx_;
    mutable std::mutex                           monitors_mtx_;

    bool worker_function();
    bool on_endpoint_data(const interface::pb::EndpointData & ep);

  public:
    log_record_client(endpoint_client & ep_client);
    ~log_record_client();
    
    void get_logs(const interface::pb::GetLogs & req,
                  std::function<bool(interface::pb::LogRecord & rec)> fun) const;
    void watch(const std::string & name, log_monitor);
    void remove_watches();
    void remove_watch(const std::string & name);
    
    bool logger_ready() const;
    bool get_logs_ready() const;
    bool subscription_ready() const;
  };
}}
