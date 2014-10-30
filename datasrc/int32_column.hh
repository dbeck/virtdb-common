#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class int32_column : public typed_column<int32_t>
  {
    typedef typed_column<int32_t> parent_type;

  public:
    int32_column(size_t max_rows);
    
    void convert_pb();
  };

  
}}
