#pragma once

#include <datasrc/string_column.hh>

namespace virtdb { namespace datasrc {
  
  class datetime_column : public string_column
  {
    typedef string_column parent_type;
    
  public:
    datetime_column(size_t max_rows);
    datetime_column(size_t max_rows, size_t max_size);
    
    void convert_pb();
  };
  
}}
