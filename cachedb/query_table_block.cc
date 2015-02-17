#include "query_table_block.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_table_block::clazz() const
  {
    static std::string clz("query_table_block");
    return clz;
  }
  
  size_t
  query_table_block::key_len() const
  {
    return 16;
  }
  
  void
  query_table_block::default_columns()
  {
    column("is_complete");
    column("#columns");
    column("#columns_complete");
  }
  
  query_table_block::query_table_block() : storeable() {}
  query_table_block::~query_table_block() {}
  
}}
