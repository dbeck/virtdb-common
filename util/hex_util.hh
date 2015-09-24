#pragma once

#include <string>

namespace virtdb { namespace util {
  
  void hex_util(unsigned long long in, std::string & out);
  void hex_util(const std::string & in, std::string & out);
  
}}
