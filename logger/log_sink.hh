#pragma once

#include <diag.pb.h>
#include <util/zmq_utils.hh>
#include <memory>

namespace virtdb { namespace logger {
  
  class log_sink final
  {
  public:
    typedef std::shared_ptr<log_sink>                   sptr;
    typedef std::shared_ptr<util::zmq_socket_wrapper>   socket_sptr;
    typedef std::shared_ptr<log_sink>                   log_sink_sptr;
    typedef std::weak_ptr<log_sink>                     log_sink_wptr;
    typedef std::shared_ptr<interface::pb::LogRecord>   pb_logrec_sptr;
    
  private:
    // hiding implementation se we break circular dependency with active_queue
    // and active_queue will be able to use logger ...
    struct queue_impl;
    std::unique_ptr<queue_impl> queue_impl_;

    log_sink();
    log_sink(const log_sink &) = delete;
    
    static log_sink_wptr      global_sink_;
    log_sink_sptr             local_sink_;
    socket_sptr               socket_;
    
    void handle_record(pb_logrec_sptr rec);
    void print_record(pb_logrec_sptr rec);
    bool socket_is_valid() const;
    
  public:
    log_sink(socket_sptr s);
    ~log_sink();
    
    bool send_record(pb_logrec_sptr rec);
    static log_sink_sptr get_sptr();
  };
  
}}
