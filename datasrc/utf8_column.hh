#pragma once

#include <datasrc/string_column.hh>

namespace virtdb { namespace datasrc {
  
  class utf8_column : public string_column
  {
    typedef string_column parent_type;
    
  public:
    utf8_column(size_t max_rows, size_t max_size);
    
    void convert_pb();
  };
  
}}
