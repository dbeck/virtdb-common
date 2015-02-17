#pragma once

#include <memory>
#include <data.pb.h>
#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class column_data : public storeable
  {
    std::unique_ptr<uint8_t[]>  data_;
    size_t                      len_;
    
  public:
    // interface:
    const std::string & clazz() const;
    size_t key_len() const;
    void default_columns();
    
    // own stuff
    void set(const interface::pb::Column & c);
    void reset();
    
    column_data();
    virtual ~column_data();
  };
  
}}
