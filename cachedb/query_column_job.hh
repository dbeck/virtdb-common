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
    
    void default_columns();
    
    query_column_job();
    virtual ~query_column_job();
  };
  
}}
