#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_column_log : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_data;
    static const qual_name qn_t0_completed_at;
    static const qual_name qn_t0_nblocks;
    static const qual_name qn_t1_completed_at;
    static const qual_name qn_t1_nblocks;
    
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();

    query_column_log();
    virtual ~query_column_log();
    
    const std::string & data() const;
    std::chrono::system_clock::time_point t0_completed_at() const;
    std::chrono::system_clock::time_point t1_completed_at() const;
    size_t t0_nblocks() const;
    size_t t1_nblocks() const;
    
    void data(const std::string & d);
    void t0_completed_at(const std::chrono::system_clock::time_point & tp);
    void t1_completed_at(const std::chrono::system_clock::time_point & tp);
    void t0_nblocks(size_t n);
    void t1_nblocks(size_t n);
  };
  
}}
