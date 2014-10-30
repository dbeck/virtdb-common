#pragma once

#include <data.pb.h>
#include <functional>
#include <memory>
#include <vector>

namespace virtdb { namespace datasrc {

  class column
  {
  public:
    typedef std::shared_ptr<column>       sptr;
    typedef std::function<void(sptr)>     on_dispose;
    typedef std::vector<bool>             null_vector;
    
  private:
    size_t                     max_rows_;
    size_t                     n_rows_;
    interface::pb::Column      pb_column_;
    on_dispose                 on_dispose_;
    null_vector                nulls_;
    
  public:
    column(size_t max_rows);
    virtual ~column() {}
    
    // common properties
    size_t max_rows() const;
    null_vector & nulls();
    void set_on_dispose(on_dispose);
    void set_last();
    void seqno(size_t seqno);
    void n_rows(size_t n);
    size_t seqno();
    bool is_last();

    // interface for children
    virtual char * get_ptr() = 0;
    virtual size_t n_rows() const;
    virtual void prepare();           // step #1: preparation
    virtual void convert_pb() = 0;    // step #2: convert internal data to uncompressed PB
    virtual void compress();          // step #3: compress data
                                      // step #4: get pb data for sending over
    virtual interface::pb::Column & get_pb_column();
    virtual void dispose(sptr);       // step #5: return this column to the pool
    
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
    typed_column(size_t max_rows)
    : column{max_rows},
      data_{new T[max_rows]}
    {}
    
    T * get_typed_ptr() { return data_.get(); }
    char * get_ptr() { return reinterpret_cast<char *>(data_.get()); }
  };
  
  class fixed_width_column : public column
  {
  public:
    typedef std::vector<size_t> size_vector;
    
  private:
    typedef column                   parent_type;
    typedef std::unique_ptr<char[]>  data_uptr;
    
    data_uptr       data_;
    size_vector     actual_sizes_;
    size_t          max_size_;
    size_t          in_field_offset_;
    
  public:
    fixed_width_column(size_t max_rows, size_t max_size);
    size_vector & actual_sizes();
    size_t max_size() const;
    char * get_ptr();
    void in_field_offset(size_t o);
    size_t in_field_offset() const;
  };
  
}}
