#pragma once

#include "barrier.hh"
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <future>
#include <exception>
#include <stdexcept>

namespace virtdb { namespace util {
  
  class async_worker final
  {
    async_worker() = delete;
    async_worker(const async_worker &) = delete;
    async_worker & operator=(const async_worker &) = delete;
    
    std::function<bool(void)>  worker_;
    barrier                    start_barrier_;
    barrier                    stop_barrier_;
    std::atomic<bool>          stop_;
    std::atomic<bool>          started_;
    std::thread                thread_;
    size_t                     n_retries_on_exception_;
    bool                       die_on_exception_;
    std::exception_ptr         error_;
    std::mutex                 error_mutex_;
    
    void entry();
    
  public:
    async_worker(std::function<bool(void)> worker,
                 size_t n_retries_on_exception=10,
                 bool die_on_exception=false);
    ~async_worker();
    void stop();
    void start();
    void rethrow_error();
  };
}}

