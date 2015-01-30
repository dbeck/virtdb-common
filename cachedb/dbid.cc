#include "dbid.hh"
#include "hash_util.hh"
#include <util/exception.hh>
#include <logger.hh>
#include <iomanip>
#include <strstream>

namespace virtdb { namespace cachedb {
  
  const std::string &
  dbid::genkey()
  {
    std::ostringstream os;
    os << objid_
       << date_time_
       << std::string(sub_a_.begin(), sub_a_.begin()+12)
       << sub_b_
       << flags_;
    key_ = os.str();
    return key_;
  }
  
  void dbid::date_now(std::string & out)
  {
    time_point tp = std::chrono::system_clock::now();
    convert_date(tp, out);
  }
  
  void dbid::convert_date(const time_point & tp,
                          std::string & out)
  {
    std::time_t  t = std::chrono::system_clock::to_time_t( tp );
    std::tm     tm = *std::localtime(&t);
    std::ostringstream os;
    os << std::put_time(&tm, "%Y%m%d%H%M%S");
    out = os.str();
  }
  
  void
  dbid::convert_flags(const std::string & in,
                      std::string & out)
  {
    std::set<char> tmp_flags;
    for( char c : in )
    {
      tmp_flags.insert(c);
      if( tmp_flags.size() == 6 )
        break;
    }
    
    static const char spaces[7] = "      ";
    
    std::string tmp_string{tmp_flags.begin(), tmp_flags.end()};
    tmp_string += spaces;
    out.assign(tmp_string.begin(), tmp_string.begin()+6);
  }
  
  // generate based on EndpointData
  dbid::dbid(const interface::pb::EndpointData & ep,
             std::string flags)
  {
    if( !ep.has_name() )
    {
      THROW_("cfg has no name");
    }
    objid_ = "@Endpoint       ";
    date_now(date_time_);
    
    if( !hash_util::hash_string(ep.name(), sub_a_) )
    {
      THROW_("failed to hash endpoint name");
    }
    
    hash_util::hex(ep.svctype(), sub_b_);
    
    convert_flags(flags, flags_);
  }
  
  // generate based on Config
  dbid::dbid(const interface::pb::Config & cfg,
             std::string flags)
  {
    if( !cfg.has_name() )
    {
      THROW_("cfg has no name");
    }
    
    objid_ = "@Config         ";
    date_now(date_time_);
    
    if( !hash_util::hash_string(cfg.name(), sub_a_) )
    {
      THROW_("failed to hash cgfg name");
    }
    
    hash_util::hex(0, sub_b_);
    
    convert_flags(flags, flags_);
  }
  
  // generate based on Query
  dbid::dbid(const interface::pb::Query & q,
             int field_id,
             time_point query_time,
             uint64_t block_id,
             std::string flags)
  {
    if( !hash_util::hash_query(q, objid_) )
    {
      THROW_("failed to hash query");
    }
    
    convert_date(query_time, date_time_);
    
    if( field_id >= q.fields_size() || field_id < 0 )
    {
      LOG_ERROR("field id out of range" << V_(field_id) << V_(q.fields_size()));
      THROW_("invalid field id. out of range.");
    }
    
    auto const & fld = q.fields(field_id);
    
    if( !hash_util::hash_field(q,fld,sub_a_) )
    {
      THROW_("failed to hash field");
    }
    
    hash_util::hex(block_id, sub_b_);
    
    convert_flags(flags, flags_);
  }
}}
