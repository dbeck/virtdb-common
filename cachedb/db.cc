#include "db.hh"

namespace virtdb { namespace cachedb {
  
  struct db::impl
  {
    int dummy_;
  };
  
  db::db() : impl_{new impl} {}
  db::~db() {}
}}
