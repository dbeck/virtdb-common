#include "query_table_block.hh"
#include <util/exception.hh>
#include <util/hex_util.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name query_table_block::qn_is_complete{query_table_block::clazz_static(),"is_complete"};
  const storeable::qual_name query_table_block::qn_n_columns{query_table_block::clazz_static(),"#columns"};
  const storeable::qual_name query_table_block::qn_n_columns_complete{query_table_block::clazz_static(),"#columns_complete"};
  
  const std::string &
  query_table_block::clazz_static()
  {
    static std::string clz("query_table_block");
    return clz;
  }
  
  const std::string &
  query_table_block::clazz() const
  {
    return clazz_static();
  }
    
  size_t
  query_table_block::key_len() const
  {
    return 16;
  }
  
  void
  query_table_block::default_columns()
  {
    column(qn_is_complete);
    column(qn_n_columns);
    column(qn_n_columns_complete);
  }
  
  query_table_block::query_table_block() : storeable() {}
  query_table_block::~query_table_block() {}
  
  void
  query_table_block::key(const std::string & hash,
                         const std::chrono::system_clock::time_point & tp,
                         size_t seq_no)
  {
    std::ostringstream os;
    std::string conv_tp;
    std::string seq_no_hex;
    if( !convert(tp,conv_tp) )
    {
      LOG_ERROR("failed to convert time_point" <<  V_(hash) << V_(seq_no) << V_(seq_no_hex));
      return;
    }
    util::hex_util(seq_no, seq_no_hex);
    os << hash << ' ' << conv_tp << ' ' << seq_no_hex;
    storeable::key(os.str());
  }
  
  bool
  query_table_block::is_complete() const
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
  query_table_block::is_complete(bool value)
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
  query_table_block::n_columns() const
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
  query_table_block::n_columns(size_t n)
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
  query_table_block::n_columns_complete() const
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
  query_table_block::n_columns_complete(size_t n)
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
  
}}
