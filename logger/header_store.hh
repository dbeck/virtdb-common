#pragma once

#include <cstdint>
#include <functional>

namespace virtdb { namespace logger {
  
  class log_record;
  
  class header_store
  {
  public:
    static uint32_t get_new_id(const log_record * rec);
    static bool has_header(const log_record * rec);
    static bool header_sent(uint32_t id);
    static void header_sent(uint32_t id, bool yes_no);
    static void reset_all();
  };
  
} }