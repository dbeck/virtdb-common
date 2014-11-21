#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "datetime_column.hh"
#include <logger.hh>

namespace virtdb { namespace datasrc {
  
  datetime_column::datetime_column(size_t max_rows)
  : parent_type{max_rows, 32}
  {
  }

  
  datetime_column::datetime_column(size_t max_rows,
                                   size_t max_size)
  : parent_type{max_rows, max_size}
  {
  }
  
  void
  datetime_column::convert_pb()
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
        
        if( !null_vals[i] && sizes[i] > 9 )
        {
          size_t act_size = sizes[i];
          char * act_ptr  = val_ptr+offset;
          int month = 10*(act_ptr[5]-'0') + (act_ptr[6]-'0');
          int day   = 10*(act_ptr[8]-'0') + (act_ptr[9]-'0');
          
          if( act_ptr[4] != '-' ||
              act_ptr[7] != '-' ||
              month < 1 || month > 12 ||
              day < 1 || day > 31 )
          {
            LOG_TRACE("not passing invalid datetime value" <<
                      V_(month) << V_(day) );
            null_vals[i] = true;
            sizes[i]     = 0;
          }
        }
        val_ptr += max_size();
      }
    }
    
    // let our parent do the real conversion
    parent_type::convert_pb();
  }
  
}}
