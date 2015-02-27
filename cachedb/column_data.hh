#pragma once

#include <data.pb.h>
#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class column_data : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_data;
    
    const std::string & clazz() const;
    
    size_t key_len() const;
    void default_columns();
    
    void set(const interface::pb::Column & c);
    size_t len() const;
    
    column_data();
    virtual ~column_data();
    
    const std::string & data() const;
  };
  
}}
