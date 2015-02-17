#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_table_job : public storeable
  {
  public:
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    query_table_job();
    virtual ~query_table_job();
  };
  
}}
