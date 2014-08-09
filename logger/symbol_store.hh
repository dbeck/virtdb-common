#pragma once

#include <cstdint>

namespace virtdb { namespace logger {
 
  class symbol_store
  {
  public:
    static uint32_t get_symbol_id(const char * str);
  };
  
}}
