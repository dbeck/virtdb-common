#pragma once

#include <cachedb/column_hash.hh>
#include <data.pb.h>

namespace virtdb { namespace cachedb {
  
  class column_hasher
  {
    column_hasher() = delete;
    ~column_hasher() = delete;
  public:
    
    static void
    hash_column(const interface::pb::Column & col_in,
                column_hash & hash_out);
  };
  
}}
