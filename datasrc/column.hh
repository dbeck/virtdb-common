#pragma once

#include <data.pb.h>
#include <functional>
#include <memory>
#include <vector>

namespace virtdb { namespace datasrc {

  class column
  {
  public:
    typedef std::shared_ptr<column>    sptr;
    typedef std::function<void(void)>  on_dispose;
    
  private:
    size_t                     max_rows_;
    interface::pb::ValueType   data_;
    on_dispose                 on_dispose_;
    
    virtual size_t n_rows_impl() const;
    
    virtual void convert_pb_impl();
    virtual void compress_impl();
    virtual interface::pb::ValueType & get_data_impl();
    virtual void dispose_impl();
    
  public:
    column(size_t max_rows, on_dispose d=[](){});
    virtual ~column() {}
    
    size_t max_rows() const;
    size_t n_rows() const;
    
    void convert_pb(); // step #1: convert internal data to uncompressed PB
    void compress();   // step #2: compress data
                       // step #3: get pb data for sending over
    interface::pb::ValueType & get_data();
    void dispose();    // step #4: return this column to the pool
    
  private:
    column() = delete;
    column(const column &) = delete;
    column & operator=(const column &) = delete;
  };
  
  template <typename T>
  class typed_column : public column
  {
  private:
    typedef std::unique_ptr<T[]> data_uptr;
    data_uptr data_;
    
  public:
    typed_column(size_t max_rows, on_dispose d=[](){})
    : column{max_rows, d},
      data_{new T[max_rows]}
    {}
    
    T * get_ptr() { return data_.get(); }
  };
  
  class fixed_width_column : public typed_column<char>
  {
  public:
    typedef std::vector<size_t> size_vector;
    
  private:
    typedef typed_column<char>  parent_type;
    size_vector  actual_sizes_;
    size_t       max_size_;
    
  public:
    fixed_width_column(size_t max_rows, size_t max_size, on_dispose d=[](){});
    size_vector & actual_sizes();
    size_t max_size() const;
  };
  
}}
