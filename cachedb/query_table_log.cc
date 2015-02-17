#include "query_table_log.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  const std::string &
  query_table_log::clazz() const
  {
    static std::string clz("query_table_log");
    return clz;
  }
  
  size_t
  query_table_log::key_len() const
  {
    return 16;
  }
  
  void
  query_table_log::default_columns()
  {
    column("data");
    column("#columns");
    column("t0_completed_at");
    column("t0_#blocks");
    column("t1_completed_at");
    column("t1_#blocks");
    column("t2_completed_at");
    column("t2_#blocks");
  }
  
  query_table_log::query_table_log() : storeable() {}
  query_table_log::~query_table_log() {}
  
}}
