#include "string_column.hh"

namespace virtdb { namespace datasrc {
  
  string_column::string_column(size_t max_rows,
                               size_t max_size,
                               on_dispose d)
  : parent_type{max_rows, max_size, d}
  {
  }
  
}}
