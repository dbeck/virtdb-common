#pragma once

#include <connector/endpoint_client.hh>
#include <util/zmq_utils.hh>
#include <connector/req_client.hh>
#include <connector/sub_client.hh>
#include <memory>
#include <map>
#include <functional>
#include <mutex>

namespace virtdb { namespace connector {
  
  class log_record_client final :
      public sub_client<interface::pb::LogRecord>
  {
    typedef sub_client<interface::pb::LogRecord>                 sub_base_type;
    
  public:
    typedef std::function<void(const std::string & name,
                               interface::pb::LogRecord & rec)>  log_monitor;
    typedef std::shared_ptr<log_record_client>                   sptr;
    
  private:
    typedef std::map<std::string, log_monitor>   monitor_map;
    typedef std::lock_guard<std::mutex>          lock;
    
    zmq::context_t                               zmqctx_;
    std::shared_ptr<util::zmq_socket_wrapper>    logger_push_socket_;
    std::shared_ptr<virtdb::logger::log_sink>    log_sink_sptr_;
    mutable std::mutex                           sockets_mtx_;

    bool worker_function();
    void on_endpoint_data(const interface::pb::EndpointData & ep);

  public:
    log_record_client(client_context::sptr ctx,
                      endpoint_client & ep_client,
                      const std::string & server_name);
    
    ~log_record_client();
    
    bool logger_ready() const;
    void wait_valid_push();
    void wait_valid_sub();

    bool wait_valid_push(uint64_t timeout_ms);
    bool wait_valid_sub(uint64_t timeout_ms);
    
    void cleanup();
    void rethrow_error();
  };
}}
