#include "int64_column.hh"

namespace virtdb { namespace datasrc {
  
  int64_column::int64_column(size_t max_rows)
  : parent_type{max_rows}
  {
  }
  
}}
