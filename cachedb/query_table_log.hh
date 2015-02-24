#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>
#include <chrono>

namespace virtdb { namespace cachedb {
  
  class query_table_log : public storeable
  {
  public:
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();

    query_table_log();
    virtual ~query_table_log();
    
    const std::string & data() const;
    size_t n_columns() const;
    std::chrono::system_clock::time_point t0_completed_at() const;
    std::chrono::system_clock::time_point t1_completed_at() const;
    std::chrono::system_clock::time_point t2_completed_at() const;
    size_t t0_nblocks() const;
    size_t t1_nblocks() const;
    size_t t2_nblocks() const;
  };
  
}}
