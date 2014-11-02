#pragma once
#include <cstdint>
#include <string>

namespace virtdb { namespace util {
  
  struct utf8
  {
    static void sanitize(char * p, size_t len);
  };
  
}}
