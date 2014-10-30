#pragma once

#include <util/exception.hh>
#include <datasrc/column.hh>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace virtdb { namespace datasrc {
  
  class pool
  {
    typedef std::list<column::sptr>       column_pool;
    typedef std::unique_lock<std::mutex>  lock;
    
    column_pool              pool_;
    size_t                   max_rows_;
    size_t                   allocated_;
    column::on_dispose       on_dispose_;
    std::condition_variable  cv_;
    std::mutex               mtx_;
    
  public:
    typedef std::shared_ptr<pool> sptr;
    
    pool(size_t max_rows);
    bool wait_all_disposed(uint64_t timeout_ms);
    size_t n_allocated();
    size_t n_disposed();
    
    template <typename COLUMN_TYPE>
    column::sptr allocate()
    {
      lock l(mtx_);
      if( pool_.size() > 0 )
      {
        auto front = pool_.front();
        pool_.pop_front();
        COLUMN_TYPE * ptr = dynamic_cast<COLUMN_TYPE *>(front.get());
        if( !ptr ) { THROW_("invalid item found in column pool"); }
        return front;
      }
      else
      {
        ++allocated_;
        column::sptr ret{new COLUMN_TYPE{max_rows_}};
        ret->set_on_dispose(on_dispose_);
        return ret;
      }
    }
    
    template <typename COLUMN_TYPE>
    column::sptr allocate(size_t max_size)
    {
      lock l(mtx_);
      if( pool_.size() > 0 )
      {
        auto front = pool_.front();
        pool_.pop_front();
        COLUMN_TYPE * ptr = dynamic_cast<COLUMN_TYPE *>(front.get());
        if( !ptr ) { THROW_("invalid item found in column pool"); }
        return front;
      }
      else
      {
        ++allocated_;
        column::sptr ret{new COLUMN_TYPE{max_rows_, max_size}};
        ret->set_on_dispose(on_dispose_);
        return ret;
      }
    }
    
    template <typename COLUMN_TYPE>
    column::sptr allocate(size_t max_size,
                          size_t in_field_offset)
    {
      lock l(mtx_);
      if( pool_.size() > 0 )
      {
        auto front = pool_.front();
        pool_.pop_front();
        COLUMN_TYPE * ptr = dynamic_cast<COLUMN_TYPE *>(front.get());
        if( !ptr ) { THROW_("invalid item found in column pool"); }
        return front;
      }
      else
      {
        ++allocated_;
        std::shared_ptr<COLUMN_TYPE> ret{new COLUMN_TYPE{max_rows_, max_size}};
        ret->set_on_dispose(on_dispose_);
        ret->in_field_offset(in_field_offset);
        return ret;
      }
    }
  };
}}
