#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class int64_column : public typed_column<int64_t>
  {
  public:
    int64_column(size_t max_rows, on_dispose d=[](){});
  };

}}
