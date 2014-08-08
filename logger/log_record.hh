#pragma once

#include "../diag.pb.h"
#include <atomic>
#include "signature.hh"
#include <iostream>
#include <map>
#include <mutex>
#include <map>

namespace virtdb { namespace logger {
  
  class log_record {
  public:
    typedef interface::pb::LogLevel log_level;
    
  private:
    uint32_t            id_;
    uint32_t            file_symbol_;
    uint32_t            line_;
    uint32_t            func_symbol_;
    log_level           level_;
    std::atomic<bool>   enabled_;
    const signature *   signature_;
    uint32_t            msg_symbol_;
    
    static uint32_t get_symbol_id(const char *);
    static uint32_t get_new_id();
    
  public:
    log_record(const char *        file,
               uint32_t            line,
               const char *        func,
               log_level           level,
               bool                enabled,
               const signature &   sig,
               const char *        msg)
    :
      id_(get_new_id()),
      file_symbol_(get_symbol_id(file)),
      line_(line),
      func_symbol_(get_symbol_id(func)),
      level_(level),
      enabled_(enabled),
      signature_(&sig),
      msg_symbol_(get_symbol_id(msg))
    {
      std::cout << msg<< "\n";
    }
    
    template <typename T>
    log_record &
    operator<<(const T &) { return *this; }
    
    void on_return() const
    {
      std::cout << "msg symbol:" << msg_symbol_ << " RET \n";
    }
    
    bool enabled() const { return enabled_; }
  };

}}
