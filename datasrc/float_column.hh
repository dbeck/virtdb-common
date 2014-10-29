#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class float_column : public typed_column<float>
  {
    typedef typed_column<float> parent_type;
    
  public:
    float_column(size_t max_rows);
  };

}}
