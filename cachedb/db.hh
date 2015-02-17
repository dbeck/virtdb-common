#pragma once

#include <memory>

namespace virtdb { namespace cachedb {
  
  class db
  {
    struct impl;
    std::unique_ptr<impl> impl_;
    
    db(const db &) = delete;
    db & operator=(const db &) = delete;
  public:
    
    db();
    virtual ~db();
  };
  
}}
