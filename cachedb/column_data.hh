#pragma once

#include <data.pb.h>
#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class column_data : public storeable
  {
    std::string  data_;
    
  public:
    // interface:
    const std::string & clazz() const;
    size_t key_len() const;
    void default_columns();
    
    // own stuff
    void set(const interface::pb::Column & c);
    void clear();
    size_t len() const;
    
    column_data();
    virtual ~column_data();
  };
  
}}
