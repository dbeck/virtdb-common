#include "query_table_job.hh"
#include <util/exception.hh>
#include <logger.hh>

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
  
  void
  query_table_job::key(const std::string hash,
                       const std::chrono::system_clock::time_point & tp)
  {
    std::ostringstream os;
    std::string conv_tp;
    if( !convert(tp,conv_tp) )
    {
      LOG_ERROR("failed to convert time_point" <<  V_(hash));
      return;
    }
    os << hash << ' ' << conv_tp;
    storeable::key(os.str());
  }
  
  bool
  query_table_job::is_complete() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_is_complete);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_is_complete.name_) );
    }
    return (ret > 0);
  }
  
  void
  query_table_job::is_complete(bool value)
  {
    size_t n = (value ? 1 : 0);
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_is_complete,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_is_complete.name_));
    }
  }
  
  
  size_t
  query_table_job::n_columns() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_n_columns);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_n_columns.name_) );
    }
    return ret;
  }
  
  void
  query_table_job::n_columns(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_n_columns,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_n_columns.name_));
    }
  }
  
  size_t
  query_table_job::n_columns_complete() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_n_columns_complete);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_n_columns_complete.name_) );
    }
    return ret;
  }
  
  void
  query_table_job::n_columns_complete(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_n_columns_complete,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_n_columns_complete.name_));
    }
  }
  
  size_t
  query_table_job::max_block() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_max_block);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_max_block.name_) );
    }
    return ret;
  }
  
  void
  query_table_job::max_block(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_max_block,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_max_block.name_));
    }
  }

  size_t
  query_table_job::n_blocks_complete() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_n_blocks_complete);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_n_blocks_complete.name_) );
    }
    return ret;
  }
  
  void
  query_table_job::n_blocks_complete(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_n_blocks_complete,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_n_blocks_complete.name_));
    }
  }

  size_t
  query_table_job::max_block_complete() const
  {
    size_t ret = 0;
    auto const & v = this->property_cref(qn_max_block_complete);
    if( v.empty() ) return ret;
    if( !convert(v,ret) )
    {
      LOG_ERROR("conversion failed" << V_(v.size()) << V_(qn_max_block_complete.name_) );
    }
    return ret;
  }
  
  void
  query_table_job::max_block_complete(size_t n)
  {
    std::string val;
    if( convert(n,val) )
    {
      this->property(qn_max_block_complete,val);
    }
    else
    {
      LOG_ERROR("conversion failed" << V_(n) << V_(qn_max_block_complete.name_));
    }
  }
  
}}
