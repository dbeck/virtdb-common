#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_column_job : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_is_complete;
    static const qual_name qn_max_block;
    static const qual_name qn_block_count;
    
    const std::string & clazz() const;
    size_t key_len() const;
    void key(const std::string hash,
             const std::chrono::system_clock::time_point & tp);
    
    void default_columns();
    
    query_column_job();
    virtual ~query_column_job();
    
    bool is_complete() const;
    size_t max_block() const;
    size_t block_count() const;
    
    void is_complete(bool value);
    void max_block(size_t n);
    void block_count(size_t n);
  };
  
}}
