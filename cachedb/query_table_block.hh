#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_table_block : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_is_complete;
    static const qual_name qn_n_columns;
    static const qual_name qn_n_columns_complete;
    
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    query_table_block();
    virtual ~query_table_block();
  };
  
}}
