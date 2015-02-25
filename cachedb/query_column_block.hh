#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_column_block : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_column_hash;
    
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    query_column_block();
    virtual ~query_column_block();
  };
  
}}
