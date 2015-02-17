#include "query_column_job.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_column_job::clazz() const
  {
    static std::string clz("query_column_job");
    return clz;
  }
  
  size_t
  query_column_job::key_len() const
  {
    return 16;
  }
  
  void
  query_column_job::default_columns()
  {
    column("is_complete");
    column("max_block#");
    column("block_count");
  }
  
  query_column_job::query_column_job() : storeable() {}
  query_column_job::~query_column_job() {}
  
}}
