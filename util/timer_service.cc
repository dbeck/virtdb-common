#include <util/timer_service.hh>
#include <iostream>

namespace virtdb { namespace util {
  
  timer_service::timer_service(uint64_t wakeup_freq_ms)
  : wakeup_freq_ms_(wakeup_freq_ms),
    worker_(std::bind(&timer_service::worker_function, this),
            /*we shall catch all exceptions*/ 10,false)
  {
    worker_.start();
  }
  
  timer_service::~timer_service()
  {
    {
      lock l(mtx_);
      condvar_.notify_all();
    }
    worker_.stop();
  }
  
  void
  timer_service::cleanup()
  {
    worker_.stop();
  }
  
  void
  timer_service::rethrow_error()
  {
    worker_.rethrow_error();
  }
  
  bool
  timer_service::worker_function()
  {
    using namespace std::chrono;
    time_point_t now = steady_clock::now();
    time_point_t max_wait = now + milliseconds{wakeup_freq_ms_};
    std::vector<item> run_these;
    
    {
      lock l(mtx_);
      
      while( !schedule_.empty() )
      {
        auto & first = schedule_.top();
        
        if( first.when_ < now )
        {
          run_these.push_back(first);
          schedule_.pop();
        }
        else
        {
          if( first.when_ < max_wait )
            max_wait = first.when_;
          break;
        }
      }
      
      if( run_these.empty() )
      {
        // we only wait if there are is no work to do for us
        condvar_.wait_until(l, max_wait);
      }
    }
    
    for( auto & it : run_these )
    {
      try
      {
        bool res = it.what_();
        if( res && it.duration_ms_ > 0 )
        {
          lock l(mtx_);
          it.when_ += milliseconds{it.duration_ms_};
          schedule_.push(it);
          // don't schedule a notification because this thread
          // will take care of the call on the next iteration
        }
      }
      catch (const std::exception & e)
      {
        std::cerr << "exception caught during timed execution" << e.what() << "\n";
      }
      catch (...)
      {
        std::cerr << "unknown exception caught during timed execution\n";
      }
    }
    
    return true;
  }
  
  void
  timer_service::schedule(const time_point_t & when,
                          function_t what)
  {
    using namespace std::chrono;
    
    time_point_t now = steady_clock::now();
    time_point_t max_wait = now + milliseconds{wakeup_freq_ms_};
    
    auto diff_ms =  duration_cast<milliseconds>(when - now).count();
    if( diff_ms > 0 )
    {
      item i{when, what, (uint64_t)diff_ms};
      {
        lock l(mtx_);
        schedule_.push(i);
        if( when < max_wait )
        {
          // only notifying the queue when we would not wakeup anyways
          condvar_.notify_one();
        }
      }
    }
    else
    {
      item i{when, what, 0};
      {
        lock l(mtx_);
        schedule_.push(i);
        // must notify because we should run this item in the past
        condvar_.notify_one();
      }
    }
  }
  
  void
  timer_service::schedule(uint64_t run_after_ms,
                          function_t what)
  {
    using namespace std::chrono;
    
    time_point_t now   = steady_clock::now();
    time_point_t when  = now + milliseconds{run_after_ms};
    time_point_t max_wait = now + milliseconds{wakeup_freq_ms_};
    
    item i{when, what, run_after_ms};
    {
      lock l(mtx_);
      schedule_.push(i);
      if( when < max_wait )
      {
        // only notifying the queue when we would not wakeup anyways
        condvar_.notify_one();
      }
    }
  }

}}
