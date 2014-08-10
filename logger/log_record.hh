#pragma once

#include "diag.pb.h"
#include "signature.hh"
#include "end_msg.hh"
#include "symbol_store.hh"
#include "header_store.hh"
#include "process_info.hh"
#include "log_sink.hh"
#include "variable.hh"
#include "util/value_type.hh"
#include <iostream>
#include <map>
#include <mutex>
#include <map>
#include <memory>
#include <cassert>

namespace virtdb { namespace logger {
  
  class log_record final
  {
  public:
    typedef interface::pb::LogLevel log_level;
    
    class sender final
    {
      typedef std::shared_ptr<interface::pb::LogRecord> pb_record_sptr;
      
      log_record              * record_;
      sender                  * root_;
      pb_record_sptr            pb_record_;
      interface::pb::LogData  * pb_data_ptr_;
      
      template <typename T>
      void add_data(const T & val,
                    interface::pb::LogData * pb_data)
      {
        auto pb_value = pb_data->add_values();
        interface::value_type<T>::set(*pb_value, val);
      }

      template <typename T>
      void add_data(const variable<T> & val,
                    interface::pb::LogData * pb_data)
      {
        auto pb_value = pb_data->add_values();
        interface::value_type<T>::set(*pb_value, *val.val_);
      }

      void add_data(const char * str,
                    interface::pb::LogData * pb_data)
      {
        // not adding C-String values, because they should already be
        // in the symbol table and handled by the header signature
      }
      
    public:
      // the last item in the list
      sender(const end_msg &, sender * parent);

      // zero items
      sender(const end_msg &, log_record * record);
      
      // the first item in the list
      template <typename T>
      sender(const T & v, log_record * record)
      : record_(record),
        root_(this),
        pb_record_(new interface::pb::LogRecord),
        pb_data_ptr_(nullptr)
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
          assert( record_ != nullptr );
          if( record_ )
          {
            auto data_array = pb_record_->mutable_data();
            pb_data_ptr_ = data_array->Add();
            pb_data_ptr_->set_headerseqno(record_->id());
            // TODO : add proper timestamp here
            pb_data_ptr_->set_elapsedmicrosec(11);
            // TODO : make this platform independent
#warning "pthread_self is not platform independet here...."
            auto thr_id = pthread_self();
            pb_data_ptr_->set_threadid(reinterpret_cast<uint64_t>(thr_id));
            add_data(v, pb_data_ptr_);
          }
        }
      }
      
      // all other items
      template <typename T>
      sender(const T & v, sender * parent)
      : record_(parent->record_),
        root_(parent->root_),
        pb_data_ptr_(parent->pb_data_ptr_)
      {
        if( pb_data_ptr_)
          add_data(v, pb_data_ptr_);
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
               const signature &        sig,
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
