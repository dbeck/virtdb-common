#pragma once

#include <cstdint>
#include <functional>

namespace virtdb { namespace logger {
 
  class symbol_store
  {
  public:
    static uint32_t get_symbol_id(const char * str);
    static uint32_t get_symbol_id(const std::string & str);
    static uint32_t max_id_sent();
    static void max_id_sent(uint32_t id);
    static bool has_more(uint32_t id);
    
    typedef std::function<bool(const std::string &, uint32_t)> symbol_iterator;
    static void for_each(symbol_iterator it, uint32_t start_id=0);
    
    static const std::string & get(uint32_t id);
  };
  
}}
