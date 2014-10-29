#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class string_column : public fixed_width_column
  {
    typedef fixed_width_column parent_type;
  public:
    string_column(size_t max_rows, size_t max_size, on_dispose d=[](){});
  };
  
}}
