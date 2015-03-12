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

  template <typename T, size_t CHECK_TIMEOUT_MS=50>
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
      row_data             data_;
      size_t               n_columns_;
      
    public:
      block(const block & other);
      block(size_t n_columns);
      
      row_data & data();
      size_t n_columns() const;
      uint64_t last_updated_ms() const;
      
      void last_updated_ms(uint64_t last_updated);
      void reset();
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
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::block(const block & other)
  : data_{other.data_},
    n_columns_{other.n_columns_}
  {
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::block(size_t n_columns)
  : data_(n_columns, item_sptr()),
    n_columns_{n_columns}
  {
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  typename table_collector<T,CHECK_TIMEOUT_MS>::row_data &
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::data()
  {
    return data_;
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::n_columns() const
  {
    return n_columns_;
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::reset()
  {
    for( auto & d : data_ )
      d.reset();
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::count_non_nil() const
  {
    size_t ret = 0;
    {
      for( auto const & r : data_ )
        if( r.get() ) ++ret;
    }
    return ret;
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::count_nil() const
  {
    size_t ret = 0;
    {
      for( auto const & r : data_ )
        if( !r.get() ) ++ret;
    }
    return ret;
  }


  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::block::set_col(size_t col_id,
                                                      item_sptr b)
  {
    data_[col_id] = b;
  }
  
  // implementation of table_collector
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::table_collector(size_t n_columns)
  : n_columns_{n_columns},
    stop_{false}
  {
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::~table_collector()
  {
    stop();
  }

  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::stop()
  {
    stop_ = true;
    cond_.notify_all();
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  bool
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::stopped() const
  {
    return stop_.load();
  }

  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::insert(size_t block_id,
                                              size_t col_id,
                                              item_ptr b)
  {
    item_sptr isptr{b};
    insert(block_id, col_id, isptr);
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::insert(size_t block_id,
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
    
    it->second.set_col(col_id, b);
    
    cond_.notify_all();
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  void
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::erase(size_t block_id)
  {
    lock l(mtx_);
    auto it = blocks_.find(block_id);
    if( it != blocks_.end() )
    {
      it->second.reset();
    }
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  typename table_collector<T,CHECK_TIMEOUT_MS>::row_data_ret
  table_collector<T,CHECK_TIMEOUT_MS>::get(size_t block_id,
                          uint64_t timeout_ms)
  {
    row_data_ret ret{row_data(n_columns_, item_sptr()), 0};
    
    {
      // initial check for block state
      lock l(mtx_);
      
      auto it = blocks_.find(block_id);
      if( it != blocks_.end() &&
          it->second.count_non_nil() == n_columns_ )
      {
        ret.first  = it->second.data();
        ret.second = it->second.count_non_nil();
        return ret;
      }
    }
    
    auto wait_till = (std::chrono::steady_clock::now() +
                      std::chrono::milliseconds(timeout_ms));
    
    while( stopped() == false )
    {
      lock l(mtx_);
      cond_.wait_for(l, std::chrono::milliseconds(CHECK_TIMEOUT_MS));
      
      auto it = blocks_.find(block_id);
      if( it != blocks_.end() )
      {
        ret.first  = it->second.data();
        ret.second = it->second.count_non_nil();
        if( ret.second == n_columns_ )
          return ret;
      }
      
      if( std::chrono::steady_clock::now() > wait_till )
      {
        break;
      }
    }
    return ret;
  }
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  uint64_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::last_updated(size_t block_id) const
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
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::missing_columns(size_t block_id) const
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
  
  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::max_block_id() const
  {
    lock l(mtx_);
    auto it = blocks_.rbegin();
    if( it == blocks_.rend() )
      return 0;
    else
      return it->first;
  }

  template <typename T, size_t CHECK_TIMEOUT_MS>
  size_t
  table_collector<T,CHECK_TIMEOUT_MS>::table_collector::n_columns() const
  {
    return n_columns_;
  }
  
}}
