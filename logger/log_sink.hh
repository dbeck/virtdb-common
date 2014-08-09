#pragma once

#include "diag.pb.h"
#include "cppzmq/zmq.hpp"
#include "util/active_queue.hh"
#include <memory>

namespace virtdb { namespace logger {
  
  class log_sink final
  {
  public:
    typedef std::shared_ptr<zmq::socket_t>             socket_sptr;
    typedef std::shared_ptr<log_sink>                  log_sink_sptr;
    typedef std::weak_ptr<log_sink>                    log_sink_wptr;
    typedef std::shared_ptr<interface::pb::LogRecord>  pb_logrec_sptr;
    typedef util::active_queue<pb_logrec_sptr>         queue;
    typedef std::unique_ptr<queue>                     queue_uptr;
    
  private:
    log_sink();
    log_sink(const log_sink &) = delete;
    
    static log_sink_wptr   global_sink_;
    log_sink_sptr          local_sink_;
    socket_sptr            socket_;
    queue_uptr             queue_;
    
    void handle_record(pb_logrec_sptr rec);
    bool socket_is_valid() const;
    
  public:
    log_sink(socket_sptr s);
    ~log_sink();
    
    bool send_record(pb_logrec_sptr rec);
    static log_sink_sptr get_sptr();
  };
  
}}