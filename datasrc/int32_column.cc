#include "int32_column.hh"

namespace virtdb { namespace datasrc {
  
  int32_column::int32_column(size_t max_rows)
  : parent_type{max_rows}
  {
  }
  
}}
