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
    typedef std::unique_lock<std::mutex>   lock;
  public:
    typedef T                               column_block;
    typedef std::shared_ptr<column_block>   column_block_sptr;
    typedef std::vector<column_block_sptr>  row_data;
    
    class block
    {
      size_t               id_;
      size_t               n_ok_;
      uint64_t             last_updated_ms_;
      row_data             data_;
      size_t               n_columns_;
      mutable std::mutex   mtx_;
      
    public:
      block(const block & other)
      : id_{other.id_},
        n_ok_{other.n_ok_},
        last_updated_ms_{other.last_updated_ms_},
        data_{other.data_},
        n_columns_{other.n_columns_}
      {
      }
      
      block(size_t id,
            size_t n_columns)
      : id_{id},
        n_ok_{0},
        last_updated_ms_{0},
        data_(n_columns, column_block_sptr()),
        n_columns_{n_columns}
      {
      }
      
      row_data & data()
      {
        return data_;
      }
      
      size_t n_columns() const
      {
        return n_columns_;
      }
      
      uint64_t last_updated_ms() const
      {
        uint64_t ret = 0;
        {
          lock l(mtx_);
          ret = last_updated_ms_;
        }
        return ret;
      }
      
      void last_updated_ms(uint64_t last_updated)
      {
        lock l(mtx_);
        last_updated_ms_ = last_updated;
      }

      void reset()
      {
        std::unique_lock<std::mutex> l(mtx_);
        n_ok_             = 0;
        last_updated_ms_  = 0;
        for( auto & d : data_ )
          d.reset();
      }
      
      bool ok() const
      {
        bool ret = false;
        {
          lock l(mtx_);
          ret = (n_ok_ == n_columns_);
        }
        return ret;
      }
      
      size_t count_non_nil() const
      {
        size_t ret = 0;
        {
          lock l(mtx_);
          for( auto const & r : data_ )
            if( r.get() ) ++ret;
        }
        return ret;
      }
      
      size_t count_nil() const
      {
        size_t ret = 0;
        {
          lock l(mtx_);
          for( auto const & r : data_ )
            if( !r.get() ) ++ret;
        }
        return ret;
      }
      
      void set_col(size_t col_id,
                   column_block_sptr b)
      {
        lock l(mtx_);
        
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
    
    row_data get(size_t block_id,
                 uint64_t timeout_ms=10000)
    {
      row_data empty;
      {
        // initial check for block state
        lock l(mtx_);
        
        auto it = blocks_.find(block_id);
        if( it != blocks_.end() && it->second.ok() )
        {
          return it->second.data();
        }
      }
      
      auto wait_till = (std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(timeout_ms));

      std::cv_status cvstat = std::cv_status::no_timeout;
    
      while( stopped() == false && cvstat != std::cv_status::timeout )
      {
        lock l(mtx_);
        cvstat = cond_.wait_until(l, wait_till);
        
        auto it = blocks_.find(block_id);
        if( it == blocks_.end() )
        {
          continue;
        }
        else if( it->second.ok() )
        {
          return it->second.data();
        }
      }
      return empty;
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
        return it->second.last_updated_ms();
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
        return it->second.count_nil();
      }
    }
    
  private:
    table_collector() = delete;
    table_collector(const table_collector &) = delete;
    table_collector & operator=(const table_collector &) = delete;
  };
  
}}
