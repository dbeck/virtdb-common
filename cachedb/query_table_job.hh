#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_table_job : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_is_complete;
    static const qual_name qn_n_columns;
    static const qual_name qn_n_columns_complete;
    static const qual_name qn_max_block;
    static const qual_name qn_n_blocks_complete;
    static const qual_name qn_max_block_complete;

    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();
    
    query_table_job();
    virtual ~query_table_job();
  };
  
}}
