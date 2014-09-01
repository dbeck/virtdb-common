#pragma once

#include <string>
#include <data.pb.h>


namespace virtdb { namespace connector {

  struct column_location
  {
    std::string query_id_;
    std::string schema_;
    std::string table_;
    std::string column_;
    
    bool operator<(const column_location & rhs);
    bool is_wildcard() const;
    bool is_wildcard_table() const;
    void init_table(const interface::pb::Query & q);
  };

}}
