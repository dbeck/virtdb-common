#pragma once

#include <data.pb.h>
#include <functional>

namespace virtdb { namespace datasrc {

  class column
  {
  public:
    typedef std::shared_ptr<column>    sptr;
    typedef std::function<void(void)>  on_dispose;
    
  private:
    size_t                     max_items_;
    interface::pb::ValueType   data_;
    on_dispose                 on_dispose_;
    
    virtual interface::pb::ValueType & get_data_impl()
    {
      return data_;
    }
    
    virtual size_t n_rows_impl() const
    {
      return 0;
    }
    
    virtual void dispose_impl()
    {
      if( on_dispose_ )
        on_dispose_();
    }
    
  public:
    
    inline size_t max_items() const
    {
      return max_items_;
    }
    
    inline interface::pb::ValueType & get_data()
    {
      return get_data_impl();
    }
    
    column(size_t max_itms,
           on_dispose d=[](){})
    : max_items_{max_itms},
      on_dispose_{d} {}
    
    inline size_t n_rows() const
    {
      return n_rows_impl();
    }
    
    inline void dispose()
    {
      dispose_impl();
    }
    
    virtual ~column() {}
    
  private:
    column() = delete;
    column(const column &) = delete;
    column & operator=(const column &) = delete;
  t};
  
  template <typename T>
  class column_t : public column
  {
  public:
  };
  
}}