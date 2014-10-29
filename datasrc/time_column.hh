#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class time_column : public fixed_width_column
  {
    typedef fixed_width_column parent_type;
    
  public:
    time_column(size_t max_rows);
  };
  
}}
