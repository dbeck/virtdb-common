#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>
#include <chrono>

namespace virtdb { namespace cachedb {
  
  class query_column_block : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_column_hash;
    
    const std::string & clazz() const;
    size_t key_len() const;
    void key(const std::string & hash,
             const std::chrono::system_clock::time_point & tp,
             size_t seq_no);
    
    void default_columns();
    
    query_column_block();
    virtual ~query_column_block();
    
    const std::string & column_hash() const;
    void column_hash(const std::string & ch);
  };
  
}}
