#include "column.hh"
#include <util/exception.hh>

namespace virtdb { namespace datasrc {

  column::column(size_t max_rows,
                 on_dispose d)
  : max_rows_{max_rows},
    on_dispose_{d}
  {
    if( !max_rows )
    {
      THROW_("max_rows parameter is zero");
    }
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
  
  fixed_width_column::fixed_width_column(size_t max_rows,
                                         size_t max_size,
                                         column::on_dispose d)
  : parent_type{max_rows*max_size, d}
  {
    if( !max_size )
    {
      THROW_("max_size parameter is zero");
    }
  }
  
  fixed_width_column::size_vector &
  fixed_width_column::actual_sizes()
  {
    return actual_sizes_;
    
  }
  
  size_t
  fixed_width_column::max_size() const
  {
    return max_size_;
  }

}}
