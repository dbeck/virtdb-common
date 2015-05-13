#pragma once

#include <util/barrier.hh>
#include <util/exception.hh>
#include <util/constants.hh>

#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <functional>
#include <vector>
#include <atomic>
#include <algorithm>
#include <iostream>

namespace virtdb { namespace util {

  template <typename ITEM, unsigned long WAKEUP_FREQ=DEFAULT_TIMEOUT_MS>
  class active_queue final
  {
  public:
    typedef std::function<void(ITEM)> item_handler;

  private:
    typedef std::mutex                   mtx;
    typedef std::queue<ITEM>             q;
    typedef std::condition_variable      cond;
    typedef std::vector<std::thread>     thread_vector;
    typedef std::atomic<bool>            flag;
    typedef std::unique_lock<mtx>        lock;
    
    active_queue() = delete;
    active_queue(const active_queue&) = delete;
    active_queue & operator=(const active_queue &) = delete;
    
    mutable mtx     mutex_;
    cond            cond_;
    mutable mtx     progress_mutex_;
    cond            progress_cond_;
    uint64_t        enqueued_;
    uint64_t        done_;
    q               queue_;
    barrier         barrier_;
    item_handler    handler_;
    thread_vector   threads_;
    flag            stop_;
    
  public:
    static const unsigned int wakeup_freq() { return WAKEUP_FREQ; }
    
    active_queue(unsigned int nthreads, item_handler handler)
    : enqueued_{0},
      done_{0},
      barrier_(nthreads+1),
      handler_(handler),
      stop_(false)
    {
      // starting all threads in the constructor
      for( unsigned int i=0; i<nthreads; ++i )
      {
        threads_.push_back(std::move(std::thread(std::bind(&active_queue::entry,this))));
      }
      
      // this won't return till all threads are ready
      barrier_.wait();
      
      // give a chance to the workers to reach wait() before this
      // thread start sending in the items
      std::this_thread::yield();
    }
    
    uint64_t n_done() const
    {
      uint64_t ret = 0;
      {
        lock l(progress_mutex_);
        ret = done_;
      }
      return ret;
    }

    uint64_t n_enqueued() const
    {
      uint64_t ret = 0;
      {
        lock l(progress_mutex_);
        ret = enqueued_;
      }
      return ret;
    }
    
    void push(const ITEM & i)
    {
      if( stopped() ) return;
      {
        lock l(progress_mutex_);
        ++enqueued_;
      }
      {
        lock l(mutex_);
        queue_.push(i);
        cond_.notify_one();
      }
    }
    
    void push(ITEM && i)
    {
      if( stopped() ) return;
      {
        lock l(progress_mutex_);
        ++enqueued_;
      }
      {
        lock l(mutex_);
        queue_.push(std::move(i));
        cond_.notify_one();
      }
    }
    
    bool stopped() const
    {
      return stop_;
    }
    
    template <typename T>
    bool wait_empty(const T & progress_for)
    {
      size_t enqueued_items = 0;
      size_t done_items     = 0;
      
      {
        lock l(progress_mutex_);
        
        enqueued_items = enqueued_;
        done_items     = done_;
      }
        
      while( enqueued_ > done_items && !stopped() )
      {
        size_t last_done = done_items;
        std::cv_status cvstat = std::cv_status::no_timeout;
        
        {
          lock l(progress_mutex_);
          
          if( enqueued_ > done_ )
          {
            // give time to the threads to progress
            cvstat = progress_cond_.wait_for(l, progress_for);
          }
          
          enqueued_items = enqueued_;
          done_items     = done_;
        }
        
        // if no progress has been made, then stop waiting for them
        if( last_done == done_items &&
            cvstat == std::cv_status::timeout )
        {
          break;
        }
      }
      
      return (enqueued_items == done_items);
    }
    
    void stop()
    {
      stop_ = true;
      cond_.notify_all();
      progress_cond_.notify_all();
      for( auto & t : threads_ )
      {
        if( t.joinable() )
          t.join();
      }
    }
    
    ~active_queue()
    {
      stop();
    }
    
  private:
    void entry()
    {
      // synchronize between the threads and the constructor
      barrier_.wait();
      
      // check if we can still run
      while( !stopped() )
      {
        ITEM tmp;
        bool has_item = false;
        {
          lock l(mutex_);
          
          if( queue_.empty() )
          {
            cond_.wait_for(l,std::chrono::milliseconds(WAKEUP_FREQ));
          }
          
          if( !queue_.empty() )
          {
            // we first dequeue the element, so we don't need to hold
            // the lock while the thread handler runs
            has_item = true;
            tmp = std::move(queue_.front());
            queue_.pop();
          }          
        }
        
        // the thread now processes the item outside the lock
        if( has_item )
        {
          try
          {
            handler_(tmp);
          }
          catch( const std::exception & e )
          {
            std::cerr << "exception caught: " << e.what() << "\n";
          }
          catch(...)
          {
            std::cerr << "unknown exception caught\n";
          }
          // signal wait_empty, no matter what the result was
          {
            lock l(progress_mutex_);
            ++done_;
            progress_cond_.notify_one();
          }
        }
      }
    }
  };

}}
