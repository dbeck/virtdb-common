#include <util/hash_file.hh>
#include <util/exception.hh>
#include <util/hex_util.hh>
#include <xxhash.h>

namespace virtdb { namespace util {
  
  bool
  hash_data(const void * p,
            size_t len,
            std::string & out)
  {
    if( !p || !len )
    {
      return false;
    }
    else
    {
      hex_util(XXH64(p, len, 0), out);
      return true;
    }
  }
  
}}
