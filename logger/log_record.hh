#pragma once

#include "../diag.pb.h"
#include "signature.hh"
#include "end_msg.hh"
#include "symbol_store.hh"
#include "header_store.hh"
#include "process_info.hh"
#include "log_sink.hh"
#include <iostream>
#include <map>
#include <mutex>
#include <map>
#include <memory>
#include <cassert>

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
      sender(const end_msg &, sender * parent);

      // zero items
      sender(const end_msg &, log_record * record);
      
      // the first item in the list
      template <typename T>
      sender(const T & v, log_record * record)
      : record_(record),
        parent_(nullptr),
        root_(this),
        pb_record_(new interface::pb::LogRecord)
      {
        {
          auto process = pb_record_->mutable_process();
          process->MergeFrom(process_info::instance().get_pb());
        }
        
        {
          uint32_t last_sent = symbol_store::max_id_sent();
          if( symbol_store::has_more(last_sent) )
          {
            auto symbols = pb_record_->mutable_symbols();
            symbol_store::for_each( [&last_sent,symbols](const std::string & symbol_str,
                                                          uint32_t symbol_id) {
              auto symbol = symbols->Add();
              symbol->set_seqno(symbol_id);
              symbol->set_value(symbol_str);
              last_sent = symbol_id;
              return true;
            }, last_sent);
            symbol_store::max_id_sent(last_sent);
          }
        }
        
        {
          assert( record_ != nullptr );
          if( record_ )
          {
            if( !header_store::header_sent(record_->id()) )
            {
              auto headers = pb_record_->mutable_headers();
              auto header_item = headers->Add();
              header_item->MergeFrom(record_->get_pb_header());
              header_store::header_sent(record_->id(),true);
            }
          }
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
      sender operator<<(const end_msg & v);
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
               const char *             msg);
    
    template <typename T>
    sender
    operator<<(const T & v)
    {
      return sender(v,this);
    }
    
    void on_return() const;
    bool enabled() const;
    uint32_t id() const;
    const interface::pb::LogHeader & get_pb_header() const;
  };

}}
