#pragma once

#include <data.pb.h>
#include <svc_config.pb.h>
#include <chrono>

namespace virtdb { namespace cachedb {
  
  class dbid final
  {
    //             / - column -           / - config -    / - endpoint -    /
    //    16 = 16  / h64(Table+Query)     / @Config       / @Endpoint       /
    // +  14 = 30  / DT                   / DT            / DT              /
    // +  12 = 42  / h64(Tab+Fld+Typ)     / h64(Name)     / h64(Name)       /
    // +  16 = 58  / Block id             / 0             / Service Type    /
    // +   6 = 64  / Max 6 flags          / ""            / ""              /
    
    std::string  objid_;      // 16 : hash or @string
    std::string  date_time_;  // 14 : YYYYMMDDHHMMSS
    std::string  sub_a_;      // 12 : hex hash64 - 4 byte
    std::string  sub_b_;      // 16 : uint64 hex
    std::string  flags_;      //  6 : unique char set
    
    std::string  key_; // result key
    
    dbid() = delete;
    dbid(const dbid &) = delete;
    dbid & operator=(const dbid &) = delete;
    
  public:
    typedef std::chrono::steady_clock::time_point time_point;

    // generate based on Query
    dbid(const interface::pb::Query & q,
         int field_id,
         time_point query_time,
         uint64_t block_id,
         std::string flags = "");
    
    // generate based on Config
    dbid(const interface::pb::Config & cfg);
    
    // generate based on EndpointData
    dbid(const interface::pb::EndpointData & ep);
    
    const std::string & genkey();
    
    virtual ~dbid() {}
    
    static void convert_date(const time_point & tp,
                             std::string & out);
  };
}}