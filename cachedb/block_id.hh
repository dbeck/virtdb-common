#pragma once

#include <data.pb.h>
#include <set>
#include <string>
#include <memory>

namespace virtdb { namespace cachedb {
  
  class block_id final
  {
    // debug data
    std::string     schema_name_;
    std::string     table_name_;
    std::string     field_name_;
    uint32_t        field_no_;
    std::string     filter_str_;
    uint64_t        limit_;
    std::string     query_date_;
    uint8_t         date_filter_len_;
    uint64_t        block_no_;
    std::set<char>  flags_;
    uint32_t        prefix_len_;
    
    // resulting database key
    std::string     db_key_;
    
    block_id() = delete;
    block_id(const block_id &) = delete;
    block_id & operatpr=(const block_id &) = delete;
    
  public:
    
    
  };
}}