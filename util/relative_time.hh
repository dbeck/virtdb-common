#pragma once

#include <chrono>

namespace virtdb { namespace util {
  
  class relative_time final
  {
  public:
    typedef std::chrono::high_resolution_clock      highres_clock;
    typedef std::chrono::time_point<highres_clock>  timepoint;
    
    relative_time();
    relative_time(const timepoint & start);
    uint64_t get_msec() const;
    uint64_t get_usec() const;
    const timepoint & started_at() const;
    
    // create one static instance as well, so this can be
    // used as a global reference timer
    static const relative_time & instance();
    
  private:
    timepoint started_at_;
    
    relative_time(const relative_time &) = delete;
    relative_time & operator=(const relative_time &) = delete;
  };
  
}}