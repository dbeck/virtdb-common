#pragma once

#include <cachedb/storeable.hh>
#include <cachedb/db.hh>

namespace virtdb { namespace cachedb {
  
  class query_column_log : public storeable
  {
  public:
    static const std::string & clazz_static();
    static const qual_name qn_data;
    static const qual_name qn_t0_completed_at;
    static const qual_name qn_t0_nblocks;
    static const qual_name qn_t1_completed_at;
    static const qual_name qn_t1_nblocks;
    
    const std::string & clazz() const;
    size_t key_len() const;
    
    void default_columns();

    query_column_log();
    virtual ~query_column_log();
  };
  
}}
