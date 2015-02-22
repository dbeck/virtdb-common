#pragma once

#include <string>
#include <set>
#include <map>
#include <functional>

namespace virtdb { namespace cachedb {
  
  class storeable
  {
  public:
    typedef std::string                    data_t;
    typedef std::set<std::string>          column_set_t;
    typedef std::map<std::string, data_t>  properties_t;
    
  private:
    std::string  key_;
    column_set_t column_set_;
    properties_t properties_;
    
    storeable & operator=(const storeable &) = delete;
    storeable(const storeable &) = delete;
    
  public:
    virtual const std::string & clazz() const = 0;
    virtual size_t key_len() const = 0;
    
    virtual const std::string & key() const;
    virtual void key(const std::string & k);
    
    virtual void default_columns() = 0;
    virtual void column(const std::string & name);
    virtual const column_set_t & column_set() const;
    
    virtual const data_t & property_cref(const std::string & name) const;
    virtual data_t property(const std::string & name) const;
    virtual void property(const std::string & name,
                          const char * ptr,
                          size_t len);
    virtual void property(const std::string & name,
                          const data_t & dta);
    virtual void properties(std::function<void(const std::string & _clazz,
                                               const std::string & _key,
                                               const std::string & _family,
                                               const data_t & _data)>) const;
    virtual void clear();
    
    storeable();
    virtual ~storeable();
  };
  
}}
