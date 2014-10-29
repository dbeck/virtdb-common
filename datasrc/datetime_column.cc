#include "datetime_column.hh"

namespace virtdb { namespace datasrc {
  
  datetime_column::datetime_column(size_t max_rows)
  : parent_type{max_rows, 32}
  {
  }
  
}}
