#include "query_column_block.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_column_block::clazz() const
  {
    static std::string clz("query_column_block");
    return clz;
  }
  
  size_t
  query_column_block::key_len() const
  {
    return 16;
  }
  
  void
  query_column_block::default_columns()
  {
    column("column_hash");
  }
  
  query_column_block::query_column_block() : storeable() {}
  query_column_block::~query_column_block() {}
  
}}
