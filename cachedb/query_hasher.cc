#include "query_hasher.hh"
#include <cachedb/hash_util.hh>
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  bool
  query_hasher::hash_query(const interface::pb::Query & query_in,
                           std::string & table_hash_out,
                           colhash_map & column_hashes_out)
  {
    // TODO
    THROW_("implement me");
    return false;
  }
}}
