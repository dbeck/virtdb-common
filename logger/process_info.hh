#pragma once

#include <chrono>
#include "diag.pb.h"

namespace virtdb { namespace logger {

  class process_info final
  {
  public:
    typedef std::chrono::high_resolution_clock      highres_clock;
    typedef std::chrono::time_point<highres_clock>  timepoint;
    
  private:
    interface::pb::ProcessInfo   pb_info_;
    timepoint                    started_at_;
    
    process_info();
  public:
    static process_info & instance();
    
    const interface::pb::ProcessInfo &
    get_pb() const
    {
      return pb_info_;
    }
    
    const timepoint &
    started_at() const
    {
      return started_at_;
    }
  };
  
}}
