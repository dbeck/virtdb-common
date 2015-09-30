#pragma once

#include <string>
#include <cinttypes>

namespace virtdb { namespace util {
  
  bool hash_data(const void * p,
                 size_t len,
                 std::string & out);
  
}}
