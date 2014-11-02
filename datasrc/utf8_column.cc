#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "utf8_column.hh"
#include <util/utf8.hh>

namespace virtdb { namespace datasrc {
  
  utf8_column::utf8_column(size_t max_rows,
                           size_t max_size)
  : parent_type{max_rows, max_size}
  {
  }
  
  void
  utf8_column::convert_pb()
  {
    // sanitize here ...
    size_t n = std::min(max_rows(), n_rows());
    auto * val_ptr    = get_ptr();
    auto & null_vals  = this->nulls();
    auto & sizes      = actual_sizes();
    size_t offset     = in_field_offset();
    auto & pb_col     = get_pb_column();
    auto * mdata      = pb_col.mutable_data();
    
    if( mdata->type() == interface::pb::Kind::STRING )
    {
      for( size_t i=0; i<n; ++i )
      {
        if( !null_vals[i] && sizes[i] > 0 )
        {
          util::utf8::sanitize(val_ptr+offset,sizes[i]);
        }
        val_ptr += max_size();
      }
    }
    
    // let our parent do the real conversion
    string_column::convert_pb();
  }
}}
