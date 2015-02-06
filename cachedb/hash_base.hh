#pragma once

#include <vector>
#include <string>

namespace virtdb { namespace cachedb {

  class hash_base
  {
    std::string value_;
    
  public:
    typedef std::pair<char *, size_t> part;
    
    void reset(const std::vector<part> & parts);
    const std::string & value() const;
    
    hash_base();
    virtual ~hash_base();
    
    bool operator<(const hash_base & other) const;
  };
  
}}
