#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class bytes_column : public fixed_width_column
  {
    typedef fixed_width_column parent_type;
    
  public:
    bytes_column(size_t max_rows, size_t max_size);
  };
  
}}
