#pragma once

#include <cachedb/query_column_hash.hh>
#include <cachedb/query_table_hash.hh>
#include <data.pb.h>
#include <map>

namespace virtdb { namespace cachedb {

  class query_hasher
  {
    query_hasher() = delete;
    ~query_hasher() = delete;
    
  public:
    typedef std::map<std::string, query_column_hash> colhash_map;
    
    static void
    hash_query(const interface::pb::Query & query_in,
               query_table_hash & table_hash_out,
               colhash_map & column_hashes_out);
  };
}}

