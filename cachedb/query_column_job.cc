#include "query_column_job.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {

  const storeable::qual_name query_column_job::qn_is_complete{query_column_job::clazz_static(),"is_complete"};
  const storeable::qual_name query_column_job::qn_max_block{query_column_job::clazz_static(),"max_block#"};
  const storeable::qual_name query_column_job::qn_block_count{query_column_job::clazz_static(),"block_count"};
  
  const std::string &
  query_column_job::clazz_static()
  {
    static std::string clz("query_column_job");
    return clz;
  }
  
  const std::string &
  query_column_job::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  query_column_job::key_len() const
  {
    return 16;
  }
  
  void
  query_column_job::default_columns()
  {
    column(qn_is_complete);
    column(qn_max_block);
    column(qn_block_count);
  }
  
  query_column_job::query_column_job() : storeable() {}
  query_column_job::~query_column_job() {}
  
}}
