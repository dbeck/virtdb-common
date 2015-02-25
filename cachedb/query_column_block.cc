#include "query_column_block.hh"
#include <util/exception.hh>
#include <util/hex_util.hh>
#include <logger.hh>
#include <sstream>

namespace virtdb { namespace cachedb {
  
  const storeable::qual_name query_column_block::qn_column_hash{query_column_block::clazz_static(),"column_hash"};
  
  const std::string &
  query_column_block::clazz_static()
  {
    static std::string clz("query_column_block");
    return clz;
  }
  
  const std::string &
  query_column_block::clazz() const
  {
    return clazz_static();
  }
  
  size_t
  query_column_block::key_len() const
  {
    return 16;
  }
  
  void
  query_column_block::default_columns()
  {
    column(qn_column_hash);
  }
  
  query_column_block::query_column_block() : storeable() {}
  query_column_block::~query_column_block() {}
  
  void
  query_column_block::key(const std::string hash,
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
  
  const std::string &
  query_column_block::column_hash() const
  {
    return this->property_cref(qn_column_hash);
  }
  
  void
  query_column_block::column_hash(const std::string & ch)
  {
    this->property(qn_column_hash, ch);
  }

}}
