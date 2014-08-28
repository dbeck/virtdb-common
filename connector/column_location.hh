#pragma once

#include <string>

namespace virtdb { namespace connector {

  struct column_location
  {
    std::string query_id_;
    std::string schema_;
    std::string table_;
    std::string column_;
    
    bool operator<(const column_location & rhs);
  };

}}
