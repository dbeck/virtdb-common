#pragma once

#include "diag.pb.h"
#include "end_msg.hh"
#include "variable.hh"
#include "symbol_store.hh"
#include "util/value_type.hh"
#include <iostream>
#include <memory>

namespace virtdb { namespace logger {
  
  class signature final
  {
  public:
    typedef std::shared_ptr<interface::pb::LogHeader> pb_header_sptr;

    class part final
    {
      signature * signature_;
      
      template <typename T>
      void add_signature(const T & val,
                         signature * sig)
      {
        auto pb_header = sig->get_pb_header();
        auto log_part = pb_header->add_parts();
        log_part->set_isvariable(false);
        log_part->set_hasdata(true);
      }
      
      template <typename T>
      void add_signature(const variable<T> & val,
                         signature * sig)
      {
        auto pb_header = sig->get_pb_header();
        auto log_part = pb_header->add_parts();
        log_part->set_isvariable(true);
        log_part->set_hasdata(true);
        log_part->set_partsymbol(symbol_store::get_symbol_id(val.name_));
      }

      void add_signature(const char * val,
                         signature * sig);
      
    public:
      // the last item in the list
      part(const end_msg &, part * parent);
      
      // zero items
      part(const end_msg &, signature * sig);

      // first part
      template <typename T>
      part(const T & val, signature * sig)
      : signature_(sig)
      {
        add_signature(val, signature_);
      }
      
      // n-th part
      template <typename T>
      part(const T & val,
           part * parent)
      : signature_(parent->signature_)
      {
        add_signature(val, signature_);
      }
      
      template <typename T>
      part
      operator<<(const T & val)
      {
        return part(val, this);
      }
    };
    
    signature();
    
    template <typename T>
    part
    operator<<(const T & val)
    {
      return part(val, this);
    }
    
    pb_header_sptr get_pb_header() const;

  private:
    pb_header_sptr       header_sptr_;
    
    signature(const signature &) = delete;
    signature& operator=(const signature&) = delete;
  };
  
}}
