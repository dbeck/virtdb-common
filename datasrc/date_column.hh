#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class date_column : public fixed_width_column
  {
    typedef fixed_width_column parent_type;
    
  public:
    date_column(size_t max_rows);
  };
  
}}
