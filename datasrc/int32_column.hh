#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class int32_column : public typed_column<int32_t>
  {
  public:
    int32_column(size_t max_rows, on_dispose d=[](){});
  };

  
}}
