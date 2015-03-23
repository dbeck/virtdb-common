#include "hash_file.hh"
#include <util/exception.hh>
#include <xxhash.h>
#include <iostream>
#include <fstream>
#include <memory>

namespace virtdb { namespace util {

  uint64_t
  hash_file(const std::string & path)
  {
    uint64_t ret = 0;
    std::ifstream ifs(path, std::ios::in|std::ios::binary|std::ios::ate);
    if( !ifs.good() || ifs.eof() || !ifs.is_open() )
    {
      return ret;
    }
    
    std::streampos size = ifs.tellg();
    std::streampos pos = 0;
    ifs.seekg (0, std::ios::beg);
    
    char buf[1024];
    XXH64_state_t base_state;
    if( XXH64_reset(&base_state, 0) != XXH_OK ) { THROW_("cannot initialize hash"); }
    
    while( pos < size )
    {
      size_t diff = size - pos;
      if( diff == 0 )   break;
      if( diff > 1024 ) diff = 1024;
      ifs.read (buf, diff);
      pos = ifs.tellg();
      if( XXH64_update(&base_state,
                       buf,
                       diff) != XXH_OK ) { THROW_("failed to update hash"); }
    }
    ifs.close();
    ret = XXH64_digest(&base_state);
    return ret;
  }
}}
