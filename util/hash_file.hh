#pragma once

#include <string>
#include <cinttypes>

namespace virtdb { namespace util {

  uint64_t hash_file(const std::string & path);
  
}}
