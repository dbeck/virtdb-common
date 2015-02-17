#include "query_hasher.hh"
#include <util/exception.hh>

namespace virtdb { namespace cachedb {
  
  void
  query_hasher::hash_query(const interface::pb::Query & query_in,
                           std::string & table_hash_out,
                           colhash_map & column_hashes_out)
  {
    // TODO
    THROW_("implement me");
  }
}}
