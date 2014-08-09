#pragma once

#include "../diag.pb.h"
#include <atomic>
#include "signature.hh"
#include "end_msg.hh"
#include <iostream>
#include <map>
#include <mutex>
#include <map>
#include <memory>

namespace virtdb { namespace logger {
  
  class log_record {
  public:
    typedef interface::pb::LogLevel log_level;
    
    class sender
    {
      typedef std::shared_ptr<interface::pb::LogRecord> pb_record_sptr_;
      
      log_record       * record_;
      sender           * parent_;
      sender           * root_;
      pb_record_sptr_    pb_record_;
      
    public:
      // the last item in the list
      sender(const end_msg &, sender * parent)
      : record_(parent->record_),
        parent_(parent),
        root_(parent->root_)
      {
        // TODO : This is the point when we can send the message out
        std::cout << "END constructor\n";
      }

      // zero items
      sender(const end_msg &, log_record * record)
      : record_(record),
        parent_(nullptr),
        root_(this)
      {
        // ignoring this one because it has no data
        // shouldn't happen at all
      }
      
      // the first item in the list
      template <typename T>
      sender(const T & v, log_record * record)
      : record_(record),
        parent_(nullptr),
        root_(this),
        pb_record_(new interface::pb::LogRecord)
      {
        // TODO : This is the point when we create the pb_record_ object
        {
          // TODO : fill the process info
          auto process = pb_record_->mutable_process();
        }
        
        {
          // TODO : fill the symbols and check what has been sent
        }
        
        {
          // TODO : fill the header and check what has been send
          //        (note: the header info here has to be created
          //               by the static object)
        }
        
        {
          // TODO : initialize data and add the current value
        }
        
        std::cout << "FIRST constructor\n";
      }
      
      // all other items
      template <typename T>
      sender(const T & v, sender * parent)
      : record_(parent->record_),
        parent_(parent),
        root_(parent->root_)
      {
        // TODO : add item into the pb message
        std::cout << "OTHER constructor\n";
      }

      // iterating over the next item
      template <typename T>
      sender
      operator<<(const T & v)
      {
        return sender(v, this);
      }
      
      // iterating over the last item
      sender
      operator<<(const end_msg & v)
      {
        return sender(v, this);
      }
    };

  private:
    uint32_t                   id_;
    uint32_t                   file_symbol_;
    uint32_t                   line_;
    uint32_t                   func_symbol_;
    log_level                  level_;
    std::atomic<bool>          enabled_;
    uint32_t                   msg_symbol_;
    interface::pb::LogHeader   header_;
    
    static uint32_t get_symbol_id(const char *);
    static uint32_t get_new_id();
    
  public:
    log_record(const char *             file,
               uint32_t                 line,
               const char *             func,
               log_level                level,
               bool                     enabled,
               const signature::part &  part,
               const char *             msg)
    :
      id_(get_new_id()),
      file_symbol_(get_symbol_id(file)),
      line_(line),
      func_symbol_(get_symbol_id(func)),
      level_(level),
      enabled_(enabled),
      msg_symbol_(get_symbol_id(msg))
    {
      // TODO : prepare the header to be included in the message
      header_.set_seqno(id_);
      header_.set_filenamesymbol(file_symbol_);
      header_.set_linenumber(line_);
      header_.set_functionnamesymbol(func_symbol_);
      header_.set_level(level_);
      header_.set_logstringsymbol(msg_symbol_);
      // TODO : fill: LogParts
      // header_.add_parts()
      // use parts here ...
      std::cout << msg<< "\n";
    }
    
    template <typename T>
    sender
    operator<<(const T & v)
    {
      return sender(v,this);
    }
    
    void on_return() const
    {
      // TODO : send a
      std::cout << "msg symbol:" << msg_symbol_ << " RET \n";
    }
    
    bool enabled() const { return enabled_; }
  };

}}
