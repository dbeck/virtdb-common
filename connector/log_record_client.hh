#pragma once

#include "endpoint_client.hh"
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
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
    typedef std::map<std::string, log_monitor>   monitor_map;
    typedef std::lock_guard<std::mutex>          lock;
    
    zmq::context_t                               zmqctx_;
    std::shared_ptr<util::zmq_socket_wrapper>    logger_push_socket_;
    util::zmq_socket_wrapper                     logger_sub_socket_;
    util::zmq_socket_wrapper                     logger_req_socket_;
    std::shared_ptr<virtdb::logger::log_sink>    log_sink_sptr_;
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
                  std::function<bool(interface::pb::LogRecord & rec)> fun);
    void watch(const std::string & name, log_monitor);
    void remove_watches();
    void remove_watch(const std::string & name);
    
    bool logger_ready() const;
    bool get_logs_ready() const;
    bool subscription_ready() const;
    
  private:
    log_record_client() = delete;
    log_record_client(const log_record_client &) = delete;
    log_record_client & operator=(const log_record_client &) = delete;
  };
}}
