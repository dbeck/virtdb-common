#pragma once

#include <data.pb.h>
#include <map>

namespace virtdb { namespace cachedb {

  class query_hasher
  {
    query_hasher() = delete;
    ~query_hasher() = delete;
    
  public:
    typedef std::map<std::string, std::string> colhash_map;
    
    static void
    hash_query(const interface::pb::Query & query_in,
               std::string & table_hash_out,
               colhash_map & column_hashes_out);
  };
}}

