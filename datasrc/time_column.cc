#include "time_column.hh"

namespace virtdb { namespace datasrc {
  
  time_column::time_column(size_t max_rows,
                           on_dispose d)
  : parent_type{max_rows, 6, d}
  {
  }
  
}}
