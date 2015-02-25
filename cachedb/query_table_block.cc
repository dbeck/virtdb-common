#include "query_table_block.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name query_table_block::qn_is_complete{query_table_block::clazz_static(),"is_complete"};
  const storeable::qual_name query_table_block::qn_n_columns{query_table_block::clazz_static(),"#columns"};
  const storeable::qual_name query_table_block::qn_n_columns_complete{query_table_block::clazz_static(),"#columns_complete"};
  
  const std::string &
  query_table_block::clazz_static()
  {
    static std::string clz("query_table_block");
    return clz;
  }
  
  const std::string &
  query_table_block::clazz() const
  {
    return clazz_static();
  }
    
  size_t
  query_table_block::key_len() const
  {
    return 16;
  }
  
  void
  query_table_block::default_columns()
  {
    column(qn_is_complete);
    column(qn_n_columns);
    column(qn_n_columns_complete);
  }
  
  query_table_block::query_table_block() : storeable() {}
  query_table_block::~query_table_block() {}
  
}}
