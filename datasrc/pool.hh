#pragma once

#include <util/exception.hh>
#include <util/relative_time.hh>
#include <util/constants.hh>
#include <datasrc/column.hh>
#include <logger.hh>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <set>
#include <atomic>

namespace virtdb { namespace datasrc {
  
  class pool
  {
    typedef std::set<column::sptr>        column_pool;
    typedef std::unique_lock<std::mutex>  lock;
    
    column_pool              pool_;
    size_t                   max_rows_;
    size_t                   allocated_;
    size_t                   max_allocated_;
    column::on_dispose       on_dispose_;
    std::condition_variable  cv_;
    std::mutex               mtx_;
    std::atomic<bool>        is_valid_;
    
  public:
    typedef std::shared_ptr<pool> sptr;
    
    pool(size_t max_rows, size_t max_allocated=2000000000); // 2 billion
    virtual ~pool();
    bool wait_all_disposed(uint64_t timeout_ms);
    size_t n_allocated();
    size_t n_disposed();
    
    template <typename COLUMN_TYPE>
    column::sptr allocate()
    {
      column::sptr ret;
      util::relative_time now;
      bool first_pass = true;
      
      while( is_valid_ )
      {
        lock l(mtx_);
        if( pool_.size() > 0 )
        {
          auto it = pool_.begin();
          ret = *it;
          pool_.erase(it);
          break;
        }
        
        if( allocated_ < max_allocated_ )
        {
          ++allocated_;
          ret.reset(new COLUMN_TYPE{max_rows_});
          ret->set_on_dispose(on_dispose_);
          break;
        }
        // we have no items int the pool and cannot
        // allocate. we have to wait for an item to be disposed
        if( first_pass ) first_pass = false;
        
        // bounded wait. we restart the loop if timed out
        cv_.wait_for(l, std::chrono::milliseconds(util::DEFAULT_TIMEOUT_MS));
      }
      
      if( !first_pass )
      {
        LOG_INFO("spent" <<  V_(now.get_msec()) << "ms waiting for column");
      }
      return ret;
    }
    
    template <typename COLUMN_TYPE>
    column::sptr allocate(size_t max_size)
    {
      column::sptr ret;
      util::relative_time now;
      bool first_pass = true;
      
      while( is_valid_ )
      {
        lock l(mtx_);
        if( pool_.size() > 0 )
        {
          auto it = pool_.begin();
          ret = *it;
          pool_.erase(it);
          break;
        }
        
        if( allocated_ < max_allocated_ )
        {
          ++allocated_;
          ret.reset(new COLUMN_TYPE{max_rows_,max_size});
          ret->set_on_dispose(on_dispose_);
          break;
        }
        // we have no items int the pool and cannot
        // allocate. we have to wait for an item to be disposed
        if( first_pass ) first_pass = false;
        
        // bounded wait. we restart the loop if timed out
        cv_.wait_for(l, std::chrono::milliseconds(util::DEFAULT_TIMEOUT_MS));
      }
      
      if( !first_pass )
      {
        LOG_INFO("spent" <<  V_(now.get_msec()) << "ms waiting for column");
      }
      return ret;
    }
    
    template <typename COLUMN_TYPE>
    column::sptr allocate(size_t max_size,
                          size_t in_field_offset)
    {
      column::sptr ret;
      util::relative_time now;
      bool first_pass = true;
      
      while( is_valid_ )
      {
        lock l(mtx_);
        if( pool_.size() > 0 )
        {
          auto it = pool_.begin();
          ret = *it;
          pool_.erase(it);
          break;
        }
        
        if( allocated_ < max_allocated_ )
        {
          ++allocated_;
          std::shared_ptr<COLUMN_TYPE> ret_tmp{new COLUMN_TYPE{max_rows_, max_size}};
          ret_tmp->set_on_dispose(on_dispose_);
          ret_tmp->in_field_offset(in_field_offset);
          ret = ret_tmp;
          break;
        }
        // we have no items int the pool and cannot
        // allocate. we have to wait for an item to be disposed
        if( first_pass ) first_pass = false;
        
        // bounded wait. we restart the loop if timed out
        cv_.wait_for(l, std::chrono::milliseconds(util::DEFAULT_TIMEOUT_MS));
      }
      
      if( !first_pass )
      {
        LOG_INFO("spent" <<  V_(now.get_msec()) << "ms waiting for column");
      }
      return ret;
    }

  private:
    pool() = delete;
    pool(const pool &) = delete;
    pool& operator=(const pool &) = delete;
  };
}}
