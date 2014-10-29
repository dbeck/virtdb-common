#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {

  class double_column : public typed_column<double>
  {
    typedef typed_column<double> parent_type;
    
  public:
    double_column(size_t max_rows);
  };
  
}}
