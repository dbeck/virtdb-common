#pragma once

#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include "relative_time.hh"
#include "exception.hh"
#include "constants.hh"

namespace virtdb { namespace util {

  template <typename T>
  class table_collector final
  {
  public:
    typedef T                               column_block;
    typedef std::shared_ptr<column_block>   column_block_sptr;
    typedef std::vector<column_block_sptr>  row_data;
    
    struct block
    {
      size_t     id_;
      size_t     n_ok_;
      uint64_t   last_updated_ms_;
      row_data   data_;
      size_t     n_columns_;
      
      block(size_t id,
            size_t n_columns)
      : id_{id},
        n_ok_{0},
        last_updated_ms_{0},
        data_(n_columns, column_block_sptr()),
        n_columns_{n_columns}
      {
      }
      
      void reset()
      {
        n_ok_             = 0;
        last_updated_ms_  = 0;
        for( auto & d : data_ )
          d.reset();
      }
      
      bool ok()
      {
        return (n_ok_ == n_columns_);
      }
      
      void set_col(size_t col_id,
                   column_block_sptr b)
      {
        auto & dptr = data_[col_id];
        if( !dptr && b )
        {
          // update counter of columns we have
          ++n_ok_;
        }
        
        // update pointer and timestamp
        dptr = b;
        last_updated_ms_ = relative_time::instance().get_msec();
      }
      
    private:
      block() = delete;
    };
    
  private:
    typedef std::unique_lock<std::mutex>   lock;
    
    size_t                         n_columns_;
    std::map<size_t, block>        blocks_;
    std::mutex                     mtx_;
    std::condition_variable        cond_;
    std::atomic<bool>              stop_;
    
  public:
    table_collector(size_t n_columns)
    : n_columns_{n_columns},
      stop_{false}
    {
    }
    
    ~table_collector()
    {
      stop();
    }
    
    void stop()
    {
      stop_ = true;
      cond_.notify_all();
    }
    
    bool stopped()
    {
      return stop_.load();
    }
    
    void insert(size_t block_id,
                size_t col_id,
                column_block_sptr b)
    {
      lock l(mtx_);
      
      // check for invalid column id
      if( n_columns_ <= col_id )
      {
        LOG_ERROR("out of bounds" << V_(n_columns_) << "<=" << V_(col_id));
        THROW_("col_id out of bounds");
      }
      
      // check if exists and create if not
      auto it = blocks_.find(block_id);
      if( it == blocks_.end() )
      {
        size_t id = blocks_.size();
        auto iit = blocks_.insert(std::make_pair(id,block(id,n_columns_)));
        it = iit.first;
      }
      
      // set column
      it->second.set_col(col_id, b);
      
      if( it->second.ok() )
      {
        // we have enough columns for this block, tell everyone waiting
        cond_.notify_all();
      }
    }
    
    void erase(size_t block_id)
    {
      lock l(mtx_);
      auto it = blocks_.find(block_id);
      if( it != blocks_.end() )
      {
        it->second.reset();
      }
    }
    
    block * get(uint64_t timeout_ms,
                size_t block_id)
    {
      auto now = std::chrono::system_clock::now();
      auto wait_till = now + std::chrono::milliseconds(timeout_ms);
      
      block * ret = nullptr;
      
      while( stopped() == false )
      {
        lock l(mtx_);
        cond_.wait_until(l,
                         wait_till,
                         [&]()
        {
          if( stopped() )
          {
            return true;
          }
          auto it = blocks_.find(block_id);
          if( it == blocks_.end() )
          {
            return false;
          }
          else if( it->second.ok() )
          {
            return true;
          }
          else
          {
            return false;
          }
        });
        
        auto it = blocks_.find(block_id);
        if(it != blocks_.end() &&
           it->second.ok())
        {
          ret = it->second.get();
          break;
        }
      }
      return ret;
    }
    
    uint64_t last_updated(size_t block_id)
    {
      lock l(mtx_);
      auto it = blocks_.find(block_id);
      if( it == blocks_.end() )
      {
        return 0;
      }
      else
      {
        return it->second.last_updated_ms_;
      }
    }
    
    size_t missing_columns(size_t block_id)
    {
      lock l(mtx_);
      auto it = blocks_.find(block_id);
      if( it == blocks_.end() )
      {
        return n_columns_;
      }
      else
      {
        if( it->second.n_ok_ > n_columns_ )
          return 0;
        else
          return (n_columns_ - it->second.n_ok_);
      }
    }
    
  private:
    table_collector() = delete;
    table_collector(const table_collector &) = delete;
    table_collector & operator=(const table_collector &) = delete;
  };
  
}}
