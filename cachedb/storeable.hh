#pragma once

#include <string>
#include <set>
#include <map>
#include <functional>

namespace virtdb { namespace cachedb {
  
  class storeable
  {
  public:
    struct data
    {
      void * buffer_;
      size_t len_;
    };
    
    typedef std::set<std::string>        column_set_t;
    typedef std::map<std::string, data>  properties_t;
    
  private:
    std::string  key_;
    column_set_t column_set_;
    properties_t properties_;
    
  public:
    virtual const std::string & clazz() const = 0;
    virtual size_t key_len() const = 0;
    
    virtual const std::string & key() const;
    virtual void key(const std::string & k);
    
    virtual void default_columns() = 0;
    virtual void column(const std::string & name);
    virtual const column_set_t & column_set() const;
    virtual data property(const std::string name) const;
    virtual void property(const std::string & name,
                          void * ptr,
                          size_t len);
    virtual void properties(std::function<void(const std::string & _clazz,
                                               const std::string & _key,
                                               const std::string & _name,
                                               data _data)>);
    
    virtual void clear();
    
    storeable();
    virtual ~storeable();
  };
  
}}
