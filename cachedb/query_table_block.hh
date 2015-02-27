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
    void key(const std::string & hash,
             const std::chrono::system_clock::time_point & tp,
             size_t seq_no);
    
    void default_columns();
    
    query_table_block();
    virtual ~query_table_block();
    
    bool is_complete() const;
    size_t n_columns() const;
    size_t n_columns_complete() const;
    
    void is_complete(bool value);
    void n_columns(size_t n);
    void n_columns_complete(size_t n);
  };
  
}}
