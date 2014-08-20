#include "async_worker.hh"

namespace virtdb { namespace util {
  
  async_worker::async_worker(std::function<bool(void)> worker)
  : worker_(worker),
    start_barrier_(2),
    stop_(false),
    thread_(std::bind(&async_worker::entry,this))
  {
  }
  
  void
  async_worker::start()
  {
    start_barrier_.wait();
  }

  void
  async_worker::stop()
  {
    stop_ = true;
    start_barrier_.wait();
  }
  
  async_worker::~async_worker()
  {
    stop();
    if( thread_.joinable() )
      thread_.join();
  }
  
  void
  async_worker::entry()
  {
    start_barrier_.wait();
    while( stop_ != true )
    {
      if( !worker_() )
        break;
    }
  }
}}
