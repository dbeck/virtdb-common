#include <util/async_worker.hh>
#include <zmq.hpp>
#include <iostream>

namespace virtdb { namespace util {
  
  async_worker::async_worker(std::function<bool(void)> worker,
                             size_t n_retries_on_exception,
                             bool die_on_exception)
  : worker_{worker},
    start_barrier_{2},
    stop_barrier_{2},
    stop_{false},
    started_{false},
    thread_{std::bind(&async_worker::entry,this)},
    n_retries_on_exception_{n_retries_on_exception},
    die_on_exception_{die_on_exception}
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
  async_worker::rethrow_error()
  {
    std::exception_ptr e;
    {
      std::lock_guard<std::mutex> l(error_mutex_);
      if( error_ )
      {
        // throw only once, so moving the exception to our temporary object
        e = {std::move(error_)};
      }
    }
    
    if( e )
    {
      // throwing outside the lock
      std::rethrow_exception(e);
    }
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
        error_ = nullptr;
      }
      catch( const zmq::error_t & e )
      {
        {
          std::lock_guard<std::mutex> l(error_mutex_);
          error_ = std::current_exception();
        }

        ++exceptions_caught;
        std::cerr << "0MQ exception caught: " << e.what()
                  << " ncaught: " << exceptions_caught
                  << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(exceptions_caught));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > n_retries_on_exception_ )
        {
          std::cerr << "stop worker loop because of too many errors. rethrowing exception: " << e.what() << "\n";
          
          if( die_on_exception_ )
            throw;
          else
            break;
        }
      }
      catch( const std::exception & e )
      {
        {
          std::lock_guard<std::mutex> l(error_mutex_);
          error_ = std::current_exception();
        }

        ++exceptions_caught;
        std::cerr << "exception caught: " << e.what()
                  << " ncaught: " << exceptions_caught
                  << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(exceptions_caught));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > n_retries_on_exception_ )
        {
          std::cerr << "stop worker loop because of too many errors. rethrowing exception: " << e.what() << "\n";
          
          if( die_on_exception_ )
            throw;
          else
            break;
        }
      }
      catch( ... )
      {
        {
          std::lock_guard<std::mutex> l(error_mutex_);
          error_ = std::current_exception();
        }

        ++exceptions_caught;
        std::cerr << "unknwon exception caught: "
                  << " ncaught: " << exceptions_caught
                  << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(exceptions_caught));
        // if we keep receiving exceptions we stop
        if( exceptions_caught > n_retries_on_exception_ )
        {
          std::cerr << "stop worker loop because of too many errors. rethrowing exception.\n";
          
          if( die_on_exception_ )
            throw;
          else
            break;
        }
      }
    }
    stop_barrier_.wait();
  }
}}
