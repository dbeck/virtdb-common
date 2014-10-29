#include "datetime_column.hh"

namespace virtdb { namespace datasrc {
  
  datetime_column::datetime_column(size_t max_rows,
                                   on_dispose d)
  : parent_type{max_rows, 32, d}
  {
  }
  
}}
