#include "query_column_block.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name query_column_block::qn_column_hash{query_column_block::clazz_static(),"column_hash"};
  
  const std::string &
  query_column_block::clazz_static()
  {
    static std::string clz("query_column_block");
    return clz;
  }
  
  const std::string &
  query_column_block::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  query_column_block::key_len() const
  {
    return 16;
  }
  
  void
  query_column_block::default_columns()
  {
    column(qn_column_hash);
  }
  
  query_column_block::query_column_block() : storeable() {}
  query_column_block::~query_column_block() {}
  
}}
