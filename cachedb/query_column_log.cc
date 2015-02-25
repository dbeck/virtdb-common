#include "query_column_log.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {

  const storeable::qual_name query_column_log::qn_data{query_column_log::clazz_static(),"data"};
  const storeable::qual_name query_column_log::qn_t0_completed_at{query_column_log::clazz_static(),"t0_completed_at"};
  const storeable::qual_name query_column_log::qn_t0_nblocks{query_column_log::clazz_static(),"t0_#blocks"};
  const storeable::qual_name query_column_log::qn_t1_completed_at{query_column_log::clazz_static(),"t1_completed_at"};
  const storeable::qual_name query_column_log::qn_t1_nblocks{query_column_log::clazz_static(),"t1_#blocks"};
  
  const std::string &
  query_column_log::clazz_static()
  {
    static std::string clz("query_column_log");
    return clz;
  }
  
  const std::string &
  query_column_log::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  query_column_log::key_len() const
  {
    return 16;
  }
  
  void
  query_column_log::default_columns()
  {
    column(qn_data);
    column(qn_t0_completed_at);
    column(qn_t0_nblocks);
    column(qn_t1_completed_at);
    column(qn_t1_nblocks);
  }
  
  query_column_log::query_column_log() : storeable() {}
  query_column_log::~query_column_log() {}
  
}}
