#include "date_column.hh"

namespace virtdb { namespace datasrc {
  
  date_column::date_column(size_t max_rows,
                           on_dispose d)
  : parent_type{max_rows, 8, d}
  {
  }
  
}}
