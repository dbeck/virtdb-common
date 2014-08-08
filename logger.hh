#pragma once

#include "diag.pb.h"
#include "logger/end_msg.hh"
#include "logger/count_items.hh"
#include "logger/signature.hh"
#include "logger/log_record.hh"
#include "logger/macros.hh"
#include "logger/on_return.hh"
#include "logger/variable.hh"
#include "util/active_queue.hh"
#include "cppzmq/zmq.hpp"
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

#define LOG_INFO(MSG) \
  LOG_COMMON_( MSG, \
               virtdb::interface::pb::LogLevel::INFO, \
               LOG_INFO_IS_ENABLED, \
               0)

#define LOG_ERROR(MSG) \
  LOG_COMMON_( MSG, \
               virtdb::interface::pb::LogLevel::ERROR, \
               LOG_ERROR_IS_ENABLED, \
               0)

#define LOG_TRACE(MSG) \
  LOG_COMMON_( MSG, \
               virtdb::interface::pb::LogLevel::SIMPLE_TRACE, \
               LOG_TRACE_IS_ENABLED, \
               0)

#define LOG_SCOPED(MSG) \
  LOG_COMMON_( MSG, \
               virtdb::interface::pb::LogLevel::SCOPED_TRACE, \
               LOG_SCOPED_IS_ENABLED, \
               1); \
               virtdb::logger::on_return LOG_INTERNAL_LOCAL_VAR(_on_return_) \
                 ( LOG_INTERNAL_LOCAL_VAR(_log_record_)  )

#define V_(VARIABLE) virtdb::logger::make_variable( #VARIABLE, VARIABLE )
