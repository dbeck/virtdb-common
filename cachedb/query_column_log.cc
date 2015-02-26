#include "query_column_log.hh"
#include <util/exception.hh>
#include <logger.hh>

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
  
  const std::string &
  query_column_log::data() const
  {
    return this->property_cref(qn_data);
  }
  
  void
  query_column_log::data(const std::string & d)
  {
    this->property(qn_data, d);
  }
  
  std::chrono::system_clock::time_point
  query_column_log::t0_completed_at() const
  {
    auto ret = std::chrono::system_clock::now() - std::chrono::hours(24*365*20); // 20y
    auto const & v = this->property_cref(qn_t0_completed_at);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_t0_completed_at.name_) );
    }
    return ret;
  }
  
  void
  query_column_log::t0_completed_at(const std::chrono::system_clock::time_point & tp)
  {
    std::string val;
    if( convert(tp,val) )
    {
      this->property(qn_t0_completed_at,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(qn_t0_completed_at.name_));
    }
  }
  
  std::chrono::system_clock::time_point
  query_column_log::t1_completed_at() const
  {
    auto ret = std::chrono::system_clock::now() - std::chrono::hours(24*365*20); // 20y
    auto const & v = this->property_cref(qn_t1_completed_at);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_t1_completed_at.name_) );
    }
    return ret;
  }
  
  void
  query_column_log::t1_completed_at(const std::chrono::system_clock::time_point & tp)
  {
    std::string val;
    if( convert(tp,val) )
    {
      this->property(qn_t1_completed_at,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(qn_t1_completed_at.name_));
    }
  }
  
  size_t
  query_column_log::t0_nblocks() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_t0_nblocks);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_t0_nblocks.name_) );
    }
    return ret;
  }
  
  void
  query_column_log::t0_nblocks(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_t0_nblocks,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_t0_nblocks.name_));
    }
  }
  
  size_t
  query_column_log::t1_nblocks() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_t1_nblocks);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_t1_nblocks.name_) );
    }
    return ret;
  }
  
  void
  query_column_log::t1_nblocks(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_t1_nblocks,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_t1_nblocks.name_));
    }
  }

  
}}
