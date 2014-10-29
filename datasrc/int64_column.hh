#pragma once

#include <datasrc/column.hh>

namespace virtdb { namespace datasrc {
  
  class int64_column : public typed_column<int64_t>
  {
    typedef typed_column<int64_t> parent_type;
    
  public:
    int64_column(size_t max_rows);
  };

}}
