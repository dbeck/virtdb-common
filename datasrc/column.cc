#include "column.hh"

namespace virtdb { namespace datasrc {

  column::column(size_t max_rows,
                 on_dispose d)
  : max_rows_{max_rows},
    on_dispose_{d}
  {
  }
  
  interface::pb::ValueType &
  column::get_data_impl()
  {
    return data_;
  }
  
  size_t
  column::n_rows_impl() const
  {
    return 0;
  }
  
  void
  column::dispose_impl()
  {
    if( on_dispose_ )
      on_dispose_();
  }
  
  void
  column::convert_pb_impl()
  {
  }
  
  void
  column::compress_impl()
  {
    // TODO
  }
  
  void
  column::compress()
  {
    compress_impl();
  }
  
  size_t
  column::max_rows() const
  {
    return max_rows_;
  }
  
  interface::pb::ValueType &
  column::get_data()
  {
    return get_data_impl();
  }
  
  size_t
  column::n_rows() const
  {
    return n_rows_impl();
  }
  
  void
  column::dispose()
  {
    dispose_impl();
  }

  void
  column::convert_pb()
  {
    convert_pb();
  }
  
}}
