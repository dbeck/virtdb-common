#include "query_table_job.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name query_table_job::qn_is_complete{query_table_job::clazz_static(),"is_complete"};
  const storeable::qual_name query_table_job::qn_n_columns{query_table_job::clazz_static(),"#columns"};
  const storeable::qual_name query_table_job::qn_n_columns_complete{query_table_job::clazz_static(),"#columns_complete"};
  const storeable::qual_name query_table_job::qn_max_block{query_table_job::clazz_static(),"max_block#"};
  const storeable::qual_name query_table_job::qn_n_blocks_complete{query_table_job::clazz_static(),"#blocks_complete"};
  const storeable::qual_name query_table_job::qn_max_block_complete{query_table_job::clazz_static(),"max_block#_complete"};
  
  const std::string &
  query_table_job::clazz_static()
  {
    static std::string clz("query_table_job");
    return clz;
  }
  
  const std::string &
  query_table_job::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  query_table_job::key_len() const
  {
    return 16;
  }
  
  void
  query_table_job::default_columns()
  {
    column(qn_is_complete);
    column(qn_n_columns);
    column(qn_n_columns_complete);
    column(qn_max_block);
    column(qn_n_blocks_complete);
    column(qn_max_block_complete);
  }
  
  query_table_job::query_table_job() : storeable() {}
  query_table_job::~query_table_job() {}
  
}}
