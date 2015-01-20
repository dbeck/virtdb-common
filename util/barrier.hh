#pragma once

#include <condition_variable>
#include <mutex>

namespace virtdb { namespace util {

  class barrier final
  {
    typedef std::mutex               mtx;
    typedef std::condition_variable  cond;
    typedef std::unique_lock<mtx>    lock;
    
    mtx            mutex_;
    cond           cond_;
    unsigned int   nwaiting_;
    unsigned int   nthreads_;
    
    barrier() = delete;
    barrier(const barrier &) = delete;
    barrier& operator=(const barrier &) = delete;
    
  public:
    barrier(unsigned int nthreads);
    ~barrier();
    void wait();
    bool wait_for(unsigned int timeout_ms);
    bool ready();
    void reset();
  };

}}
