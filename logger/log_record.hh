#pragma once

#include "../diag.pb.h"
#include <atomic>
#include "signature.hh"
#include <iostream>

namespace virtdb { namespace logger {
  
  class log_record {
  public:
    typedef interface::pb::LogLevel log_level;
    
  private:
    uint32_t            id_;
    std::string         file_;
    uint32_t            file_symbol_;
    uint32_t            line_;
    std::string         func_;
    uint32_t            func_symbol_;
    log_level           level_;
    std::atomic<bool>   enabled_;
    const signature *   signature_;
    std::string         msg_string_;
    uint32_t            msg_symbol_;
    
  public:
    log_record(const char *        file,
               uint32_t            line,
               const char *        func,
               log_level           level,
               bool                enabled,
               const signature &   sig,
               const char *        msg)
    : id_(0), // TODO
      file_(file), file_symbol_(0), // TODO
      line_(line),
      func_(func), func_symbol_(0), // TODO
      level_(level),
      enabled_(enabled),
      signature_(&sig),
      msg_string_(msg), msg_symbol_(0) // TODO
    {
      std::cout << msg_string_ << "\n";
    }
    
    template <typename T>
    log_record &
    operator<<(const T &) { return *this; }
    
    void on_return() const
    {
      std::cout << msg_string_ << " RET \n";
    }
    
    bool enabled() const { return enabled_; }
  };

}}
