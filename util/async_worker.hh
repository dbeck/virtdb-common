#pragma once

#include "barrier.hh"
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>

namespace virtdb { namespace util {
  
  class async_worker final
  {
    async_worker() = delete;
    async_worker(const async_worker &) = delete;
    async_worker & operator=(const async_worker &) = delete;
    
    std::function<bool(void)>  worker_;
    barrier                    start_barrier_;
    std::atomic<bool>          stop_;
    std::thread                thread_;
    
    void entry();
    
  public:
    async_worker(std::function<bool(void)> worker);
    ~async_worker();
    void stop();
    void start();
  };
}}

