#include "query_column_log.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_column_log::clazz() const
  {
    static std::string clz("query_column_log");
    return clz;
  }
  
  size_t
  query_column_log::key_len() const
  {
    return 16;
  }
  
  void
  query_column_log::default_columns()
  {
    column("data");
    column("t0_completed_at");
    column("t0_#blocks");
    column("t1_completed_at");
    column("t1_#blocks");
    column("t2_completed_at");
    column("t2_#blocks");
  }
  
  query_column_log::query_column_log() : storeable() {}
  query_column_log::~query_column_log() {}
  
}}
