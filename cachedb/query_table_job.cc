#include "query_table_job.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_table_job::clazz() const
  {
    static std::string clz("query_table_job");
    return clz;
  }
  
  size_t
  query_table_job::key_len() const
  {
    return 16;
  }
  
  void
  query_table_job::default_columns()
  {
    column("is_complete");
    column("#columns");
    column("#columns_complete");
    column("max_block#");
    column("#block_comple");
    column("max_block#_complete");
  }
  
  query_table_job::query_table_job() : storeable() {}
  query_table_job::~query_table_job() {}
  
}}
