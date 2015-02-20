#pragma once

#include <data.pb.h>
#include <meta_data.pb.h>

namespace virtdb { namespace cachedb {
  
  class hash_util final
  {
  public:
    typedef std::map<std::string, std::string> colhash_map;
    
    static bool hash_query(const interface::pb::Query & query_in,
                           std::string & table_hash_out,
                           colhash_map & column_hashes_out);
    
    static bool hash_data(const void * p,
                          size_t len,
                          std::string & out);
  };
  
}}