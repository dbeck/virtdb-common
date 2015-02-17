#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class column_data : public storeable
  {
  public:
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    column_data();
    virtual ~column_data();
  };
  
}}
