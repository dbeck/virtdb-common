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
  
  const std::string &
  query_table_log::data() const
  {
    return this->property_cref(qual_name{"data"});
  }
  
  size_t
  query_table_log::n_columns() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qual_name{"#columns"});
    if( v.empty() ) return ret;
    return ret;
  }
  
  std::chrono::system_clock::time_point
  query_table_log::t0_completed_at() const
  {
    auto ret = std::chrono::system_clock::now() - std::chrono::hours(24*365*20); // 20y
    auto const & v = this->property_cref(qual_name{"t0_completed_at"});
    if( v.empty() ) return ret;
    
    auto x = std::chrono::system_clock::now();
    
    return ret;
  }
  
  std::chrono::system_clock::time_point
  query_table_log::t1_completed_at() const
  {
    auto ret = std::chrono::system_clock::now() - std::chrono::hours(24*365*20); // 20y
    auto const & v = this->property_cref(qual_name{"t1_completed_at"});
    if( v.empty() ) return ret;
    return ret;
  }
  
  std::chrono::system_clock::time_point
  query_table_log::t2_completed_at() const
  {
    auto ret = std::chrono::system_clock::now() - std::chrono::hours(24*365*20); // 20y
    auto const & v = this->property_cref(qual_name{"t2_completed_at"});
    if( v.empty() ) return ret;
    return ret;
  }
  
  size_t
  query_table_log::t0_nblocks() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qual_name{"t0_#blocks"});
    if( v.empty() ) return ret;
    return ret;
  }
  
  size_t
  query_table_log::t1_nblocks() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qual_name{"t1_#blocks"});
    if( v.empty() ) return ret;
    return ret;
  }
  
  size_t
  query_table_log::t2_nblocks() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qual_name{"t2_#blocks"});
    if( v.empty() ) return ret;
    return ret;
  }
  
}}
