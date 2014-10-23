#include "async_worker.hh"
#include <logger.hh>
#include <zmq.hpp>

namespace virtdb { namespace util {
  
  async_worker::async_worker(std::function<bool(void)> worker)
  : worker_(worker),
    start_barrier_(2),
    stop_barrier_(2),
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
    if( !started_ )
    {
      start_barrier_.wait();
    }
    stop_barrier_.wait();
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
    started_ = true;
    size_t exceptions_caught = 0;
    while( stop_ != true )
    {
      try
      {
        if( !worker_() )
          break;
        
        // reset exception counter
        exceptions_caught = 0;
      }
      catch( const zmq::error_t & e )
      {
        ++exceptions_caught;
        LOG_ERROR("0MQ exception caught" << E_(e) << V_(exceptions_caught));
        std::this_thread::sleep_for(std::chrono::milliseconds(exceptions_caught*100));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > 10 )
        {
          LOG_ERROR("stop worker loop because of too many errors");
          break;
        }
      }
      catch( const std::exception & e )
      {
        ++exceptions_caught;
        LOG_ERROR("exception caught" << E_(e) << V_(exceptions_caught));
        std::this_thread::sleep_for(std::chrono::milliseconds(exceptions_caught*100));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > 10 )
        {
          LOG_ERROR("stop worker loop because of too many errors");
          break;
        }
      }
      catch( ... )
      {
        ++exceptions_caught;
        LOG_ERROR("unknown excpetion caught" << V_(exceptions_caught));
        std::this_thread::sleep_for(std::chrono::milliseconds(exceptions_caught*100));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > 10 )
        {
          LOG_ERROR("stop worker loop because of too many errors");
          break;
        }
      }
    }
    stop_barrier_.wait();
  }
}}
