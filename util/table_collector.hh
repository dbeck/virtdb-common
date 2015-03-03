#pragma once

#include <map>
#include <vector>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <util/relative_time.hh>
#include <util/exception.hh>
#include <util/constants.hh>
#include <logger.hh>

namespace virtdb { namespace util {

  template <typename T>
  class table_collector final
  {
    typedef std::unique_lock<std::mutex>     lock;
  public:
    typedef T                                 item;
    typedef std::shared_ptr<item>             item_sptr;
    typedef T*                                item_ptr;
    typedef std::vector<item_sptr>            row_data;
    typedef std::pair<row_data,size_t>        row_data_ret;
    typedef std::shared_ptr<table_collector>  sptr;
    
    class block
    {
      size_t               n_ok_;
      uint64_t             last_updated_ms_;
      row_data             data_;
      size_t               n_columns_;
      mutable std::mutex   mtx_;
      
    public:
      block(const block & other);
      block(size_t n_columns);
      
      row_data & data();
      size_t n_columns() const;
      uint64_t last_updated_ms() const;
      
      void last_updated_ms(uint64_t last_updated);
      void reset();
      bool ok() const;
      size_t n_ok() const;
      size_t count_non_nil() const;
      size_t count_nil() const;

      void set_col(size_t col_id,
                   item_sptr b);
      
    private:
      block() = delete;
    };
    
    table_collector(size_t n_columns);
    virtual ~table_collector();
    void stop();
    bool stopped() const;
    void insert(size_t block_id,
                size_t col_id,
                item_sptr b);
    void insert(size_t block_id,
                size_t col_id,
                item_ptr b);
    void erase(size_t block_id);
    row_data_ret get(size_t block_id,
                     uint64_t timeout_ms=10000);
    uint64_t last_updated(size_t block_id) const;
    size_t missing_columns(size_t block_id) const;
    size_t max_block_id() const;
    size_t n_columns() const;
    
  private:
    table_collector() = delete;
    table_collector(const table_collector &) = delete;
    table_collector & operator=(const table_collector &) = delete;
    
    size_t                         n_columns_;
    std::map<size_t, block>        blocks_;
    mutable std::mutex             mtx_;
    std::condition_variable        cond_;
    std::atomic<bool>              stop_;
  };
  
  // implementation of table_collector::block
  
  template <typename T>
  table_collector<T>::table_collector::block::block(const block & other)
  : n_ok_{other.n_ok_},
    last_updated_ms_{other.last_updated_ms_},
    data_{other.data_},
    n_columns_{other.n_columns_}
  {
  }
  
  template <typename T>
  table_collector<T>::table_collector::block::block(size_t n_columns)
  : n_ok_{0},
    last_updated_ms_{0},
    data_(n_columns, item_sptr()),
    n_columns_{n_columns}
  {
  }
  
  template <typename T>
  typename table_collector<T>::row_data &
  table_collector<T>::table_collector::block::data()
  {
    return data_;
  }
  
  template <typename T>
  size_t
  table_collector<T>::table_collector::block::n_columns() const
  {
    return n_columns_;
  }

  template <typename T>
  size_t
  table_collector<T>::table_collector::block::n_ok() const
  {
    return n_ok_;
  }
  
  template <typename T>
  uint64_t
  table_collector<T>::table_collector::block::last_updated_ms() const
  {
    uint64_t ret = 0;
    {
      lock l(mtx_);
      ret = last_updated_ms_;
    }
    return ret;
  }

  template <typename T>
  void
  table_collector<T>::table_collector::block::last_updated_ms(uint64_t last_updated)
  {
    lock l(mtx_);
    last_updated_ms_ = last_updated;
  }
  
  template <typename T>
  void
  table_collector<T>::table_collector::block::reset()
  {
    std::unique_lock<std::mutex> l(mtx_);
    n_ok_             = 0;
    last_updated_ms_  = 0;
    for( auto & d : data_ )
      d.reset();
  }
  
  template <typename T>
  bool
  table_collector<T>::table_collector::block::ok() const
  {
    bool ret = false;
    {
      lock l(mtx_);
      ret = (n_ok_ == n_columns_);
    }
    return ret;
  }
  
  template <typename T>
  size_t
  table_collector<T>::table_collector::block::count_non_nil() const
  {
    size_t ret = 0;
    {
      lock l(mtx_);
      for( auto const & r : data_ )
        if( r.get() ) ++ret;
    }
    return ret;
  }
  
  template <typename T>
  size_t
  table_collector<T>::table_collector::block::count_nil() const
  {
    size_t ret = 0;
    {
      lock l(mtx_);
      for( auto const & r : data_ )
        if( !r.get() ) ++ret;
    }
    return ret;
  }


  template <typename T>
  void
  table_collector<T>::table_collector::block::set_col(size_t col_id,
                                                      item_sptr b)
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
  
  // implementation of table_collector
  
  template <typename T>
  table_collector<T>::table_collector::table_collector(size_t n_columns)
  : n_columns_{n_columns},
    stop_{false}
  {
  }
  
  template <typename T>
  table_collector<T>::table_collector::~table_collector()
  {
    stop();
  }

  template <typename T>
  void
  table_collector<T>::table_collector::stop()
  {
    stop_ = true;
    cond_.notify_all();
  }
  
  template <typename T>
  bool
  table_collector<T>::table_collector::stopped() const
  {
    return stop_.load();
  }

  template <typename T>
  void
  table_collector<T>::table_collector::insert(size_t block_id,
                                              size_t col_id,
                                              item_ptr b)
  {
    item_sptr isptr{b};
    insert(block_id, col_id, isptr);
  }
  
  template <typename T>
  void
  table_collector<T>::table_collector::insert(size_t block_id,
                                              size_t col_id,
                                              item_sptr b)
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
      auto iit = blocks_.insert(std::make_pair(block_id,block(n_columns_)));
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
  
  template <typename T>
  void
  table_collector<T>::table_collector::erase(size_t block_id)
  {
    lock l(mtx_);
    auto it = blocks_.find(block_id);
    if( it != blocks_.end() )
    {
      it->second.reset();
    }
  }
  
  template <typename T>
  typename table_collector<T>::row_data_ret
  table_collector<T>::get(size_t block_id,
                          uint64_t timeout_ms)
  {
    row_data_ret ret;
    {
      // initial check for block state
      lock l(mtx_);
      
      auto it = blocks_.find(block_id);
      if( it != blocks_.end() && it->second.ok() )
      {
        ret.first  = it->second.data();
        ret.second = it->second.n_ok();
        return ret;
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
      else if( it->second.ok() || cvstat == std::cv_status::timeout )
      {
        ret.first  = it->second.data();
        ret.second = it->second.n_ok();
        return ret;
      }
    }
    return ret;
  }
  
  template <typename T>
  uint64_t
  table_collector<T>::table_collector::last_updated(size_t block_id) const
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
  
  template <typename T>
  size_t
  table_collector<T>::table_collector::missing_columns(size_t block_id) const
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
  
  template <typename T>
  size_t
  table_collector<T>::table_collector::max_block_id() const
  {
    lock l(mtx_);
    auto it = blocks_.rbegin();
    if( it == blocks_.rend() )
      return 0;
    else
      return it->first;
  }

  template <typename T>
  size_t
  table_collector<T>::table_collector::n_columns() const
  {
    return n_columns_;
  }
  
}}
