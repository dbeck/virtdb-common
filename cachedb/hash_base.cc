#include "hash_base.hh"
#include <util/exception.hh>
#include <xxhash.h>

namespace virtdb { namespace cachedb {
  
  void
  hash_base::reset(const std::vector<part> & parts)
  {
    // TODO
    THROW_("implement me");
  }
  
  const std::string &
  hash_base::value() const
  {
    return value_;
  }
  
  hash_base::hash_base() : value_{"0000000000000000"} {}
  
  hash_base::~hash_base() {}
  
  bool
  hash_base::operator<(const hash_base & other) const
  {
    return this->value_ < other.value_;
  }

}}
