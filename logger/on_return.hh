#pragma once

#include <logger/log_record.hh>

namespace virtdb { namespace logger {

  class on_return final
  {
    const log_record * log_record_;
    
  public:
    on_return() : log_record_{0}
    {
    }
    
    on_return(const log_record * r) : log_record_{r}
    {
    }
    
    ~on_return()
    {
      if( log_record_ )
        log_record_->on_return();
    }
  };

}}
