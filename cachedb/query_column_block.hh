#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_column_block : public storeable
  {
  public:
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    query_column_block();
    virtual ~query_column_block();
  };
  
}}
