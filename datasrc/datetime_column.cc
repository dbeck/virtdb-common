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
          char * act_ptr  = val_ptr+offset;
          int month = 10*(act_ptr[5]-'0') + (act_ptr[6]-'0');
          int day   = 10*(act_ptr[8]-'0') + (act_ptr[9]-'0');
          int year  = ::atoi(act_ptr);
          
          static int day_counts[13] = {
            0,  // none
            31, // jan
            28, // feb
            31, // mar
            30, // apr
            31, // may
            30, // jun
            31, // jul
            31, // aug
            30, // sep
            31, // oct
            30, // nov
            31, // dec
          };
          
          if( act_ptr[4] != '-'        ||
              act_ptr[7] != '-'        ||
              month < 1 || month > 12  ||
              day < 1   || day > 31)
          {
            LOG_TRACE("not passing invalid datetime value" <<
                      V_(year) << V_(month) << V_(day) );
            null_vals[i] = true;
            sizes[i]     = 0;
          }
          else if( month == 2 && day == 29 )
          {
            bool ok = false;
            if( year < 1600 )            ok = true;
            else if( (year % 400) == 0 ) ok = true;
            else if( (year % 100) == 0 ) ok = false;
            else if( (year % 4) == 0 )   ok = true;
            if( !ok )
            {
              LOG_TRACE("not passing invalid datetime value" <<
                        V_(year) << V_(month) << V_(day) <<
                        "not a leap year");
              null_vals[i] = true;
              sizes[i]     = 0;
            }
          }
          else if( day > day_counts[month] )
          {
            LOG_TRACE("not passing invalid datetime value" <<
                      V_(year) << V_(month) << V_(day) <<
                      "no such day in this month" );
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
