#pragma once

#include <map>
#include <vector>
#include <memory>
#include <util/relative_time.hh>

namespace virtdb { namespace util {

  template <typename T>
  class data_queue final
  {
  public:
    typedef T                               column_block;
    typedef std::shared_ptr<column_block>   column_block_sptr;
    typedef std::vector<column_block_sptr>  row_data;
    
    struct item
    {
      size_t     id_;
      size_t     n_ok_;
      uint64_t   last_updated_ms_;
      row_data   data_;
      
      item(size_t id) : id_{id}, n_ok_{0}, last_updated_ms_{0} {}
    private:
      item() = delete;
    };
    
  private:
    size_t                   n_columns_;
    std::map<size_t, item>   blocks_;
    
  public:
    data_queue(size_t n_columns) : n_columns_(n_columns) {}
    ~data_queue() {}
    
    void insert(size_t block_id, size_t col_id, column_block_sptr b)
    {
    }
    
    void release(size_t block_id)
    {
    }
    
    item * get(uint64_t timeout_ms, size_t block_id)
    {
    }
    
    uint64_t last_updated(size_t block_id)
    {
    }
    
  private:
    data_queue() = delete;
    data_queue(const data_queue &) = delete;
    data_queue & operator=(const data_queue &) = delete;
  };
  
}}
