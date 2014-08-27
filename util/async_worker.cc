#include "async_worker.hh"
#include <logger.hh>
#include <zmq.hpp>

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
      try
      {
        if( !worker_() )
          break;
      }
      catch( const zmq::error_t & e )
      {
        std::string text{e.what()};
        LOG_ERROR("0MQ exception caught" << V_(text));
      }
      catch( const std::exception & e )
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown excpetion caught");
      }
    }
  }
}}
