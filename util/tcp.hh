#pragma once

#include <string>

namespace virtdb { namespace util {

  struct tcp
  {
    static unsigned short find_unused_port(const std::string & hostname="*");
  };
}}
