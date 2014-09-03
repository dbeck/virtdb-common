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
#include "util/relative_time.hh"
#include <iostream>
#include <map>
#include <mutex>
#include <map>
#include <memory>
#include <cassert>
#include <atomic>
#include <functional>
#include <thread>

namespace virtdb { namespace logger {
  
  class log_record final
  {
  public:
    typedef interface::pb::LogLevel log_level;
    
    class sender final
    {
      typedef std::shared_ptr<interface::pb::LogRecord> pb_record_sptr;
      
      const log_record        * record_;
      sender                  * root_;
      pb_record_sptr            pb_record_;
      interface::pb::LogData  * pb_data_ptr_;

      void add_data(const char * str,
                    interface::pb::LogData * pb_data)
      {
        // not adding C-String values, because they should already be
        // in the symbol table and handled by the header signature
      }
      
      template <typename T>
      void add_data(const T & val,
                    interface::pb::LogData * pb_data)
      {
        auto pb_value = pb_data->add_values();
        util::value_type<T>::set(*pb_value, val);
      }

      template <typename T>
      void add_data(const variable<T> & val,
                    interface::pb::LogData * pb_data)
      {
        auto pb_value = pb_data->add_values();
        util::value_type<T>::set(*pb_value, *val.val_);
      }
      
    public:
      void prepare_record();
      
      // the last item in the list
      sender(const end_msg &, sender * parent);

      // return from scoped message
      sender(const end_msg &, const log_record * record);
      
      // the first item in the list
      template <typename T>
      sender(const T & v,
             const log_record * record)
      : record_(record),
        root_(this),
        pb_record_(new interface::pb::LogRecord),
        pb_data_ptr_(nullptr)
      {
        prepare_record();
  
        {
          assert( record_ != nullptr );
          if( record_ )
          {
            pb_data_ptr_ = pb_record_->add_data();
            pb_data_ptr_->set_headerseqno(record_->id());
            pb_data_ptr_->set_elapsedmicrosec(util::relative_time::instance().get_usec());
            std::hash<std::thread::id> hash_fn;
            std::size_t thr_hash = hash_fn(std::this_thread::get_id());        
            pb_data_ptr_->set_threadid(static_cast<uint64_t>(thr_hash));
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
    operator<<(const T & v) const
    {
      return sender(v,this);
    }
    
    void on_return() const;
    const interface::pb::LogHeader & get_pb_header() const;
    
    uint32_t   id()           const { return id_;           }
    uint32_t   file_symbol()  const { return file_symbol_;  }
    uint32_t   line()         const { return line_;         }
    uint32_t   func_symbol()  const { return func_symbol_;  }
    log_level  level()        const { return level_;        }
    bool       enabled()      const { return enabled_;      }
  };

}}
