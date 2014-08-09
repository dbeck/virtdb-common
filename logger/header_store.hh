#pragma once

#include <cstdint>
#include <functional>

namespace virtdb { namespace logger {
  
  class log_record;
  
  class header_store
  {
  public:
    static uint32_t get_new_id(log_record * rec);
    static bool header_sent(uint32_t id);
    static void header_sent(uint32_t id, bool yes_no);
    
    /*
    static uint32_t get_symbol_id(const char * str);
    static uint32_t max_id_sent();
    static void max_id_sent(uint32_t id);
    static bool has_more(uint32_t id);
    
    typedef std::function<bool(const std::string &, uint32_t)> symbol_iterator;
    static void for_each(symbol_iterator it, uint32_t start_id=0);
     */
  };
  
} }