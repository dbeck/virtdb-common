#pragma once

#include <functional>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <util/async_worker.hh>

namespace virtdb { namespace util {
  
  class timer_service final
  {
  public:
    typedef std::chrono::steady_clock::time_point time_point_t;
    typedef std::function<bool(void)>             function_t;
    
  private:
    struct item
    {
      time_point_t  when_;
      function_t    what_;
      uint64_t      duration_ms_;
      
      bool operator<(const item & other) const
      {
        return when_ > other.when_;
      }
    };

    typedef std::unique_lock<std::mutex> lock;
    
    uint64_t                       wakeup_freq_ms_;
    std::priority_queue<item>      schedule_;
    std::mutex                     mtx_;
    std::condition_variable        condvar_;
    async_worker                   worker_;
    
    bool worker_function();

  public:
    timer_service(uint64_t wakeup_freq_ms=30000);
    ~timer_service();
    
    void schedule(const time_point_t & when,
                  function_t what);
    
    void schedule(uint64_t run_after_ms,
                  function_t what);
    
    void cleanup();
    void rethrow_error();
    
  private:
    timer_service(const timer_service &) = delete;
    timer_service& operator=(const timer_service &) = delete;
  };
}}
