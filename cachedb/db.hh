#pragma once

#include <cachedb/storeable.hh>
#include <memory>
#include <vector>
#include <string>
#include <set>

namespace virtdb { namespace cachedb {
  
  class db
  {
    struct impl;
    std::unique_ptr<impl> impl_;
    
    db(const db &) = delete;
    db & operator=(const db &) = delete;
    
  public:
    typedef std::shared_ptr<db>      sptr;
    typedef std::vector<storeable *> storeable_ptr_vec_t;
    
    bool init(const std::string & path,
              const storeable_ptr_vec_t & stvec);
    
    bool set(const storeable & data);
    bool remove(const storeable & data);
    bool exists(const storeable & data);
    
    std::set<std::string> column_families() const;
    
    db();
    virtual ~db();
  };
  
}}
