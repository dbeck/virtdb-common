#include "dbid.hh"
#include "hash_util.hh"
#include <util/exception.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {

#if 0
  //             / - column -           / - config -    / - endpoint -    /
  //    16 = 16  / h64(Table+Query)     / @Config       / @Endpoint       /
  // +  14 = 30  / DT                   / DT or 99YY    / DT or 99YY      /
  // +  12 = 42  / h64(Tab+Fld+Typ)     / h64(Name)     / h64(Name)       /
  // +  16 = 58  / Block id             / 0             / Service Type    /
  // +   6 = 64  / Max 6 flags          / ""            / ""              /
  
  std::string  objid_;      // 16 : hash or @string
  std::string  date_time_;  // 14 : YYYYMMDDHHMMSS
  std::string  sub_a_;      // 12 : hex hash64 - 4 byte
  std::string  sub_b_;      // 16 : uint64 hex
  std::string  flags_;      //  6 : unique char set
  
  std::string  key_; // result key
#endif
  
  void dbid::convert_date(const time_point & tp,
                          std::string & out)
  {
  }
  
  // generate based on EndpointData
  dbid::dbid(const interface::pb::EndpointData & ep)
  {
    if( !ep.has_name() )
    {
      THROW_("cfg has no name");
    }
    
    objid_ = "@Config";
    
    time_point tp = std::chrono::steady_clock::now();
    convert_date(tp, date_time_);
    
    if( !hash_util::hash_string(ep.name(), sub_a_) )
    {
      THROW_("failed to hash cgfg name");
    }
    
    hash_util::hex(0, sub_b_);
  }
  
  // generate based on Config
  dbid::dbid(const interface::pb::Config & cfg)
  {
    if( !cfg.has_name() )
    {
      THROW_("cfg has no name");
    }
    
    objid_ = "@Config";
    
    time_point tp = std::chrono::steady_clock::now();
    convert_date(tp, date_time_);
    
    if( !hash_util::hash_string(cfg.name(), sub_a_) )
    {
      THROW_("failed to hash cgfg name");
    }
    
    hash_util::hex(0, sub_b_);
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
    
    flags_ = flags;
  }
  
  const std::string &
  dbid::genkey()
  {
    return key_;
  }
}}
