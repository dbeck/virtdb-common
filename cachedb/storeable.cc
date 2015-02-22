#include "storeable.hh"
#include <sstream>

namespace virtdb { namespace cachedb {
  
  storeable::storeable() {}
  storeable::~storeable() {}
  

  
  const std::string &
  storeable::key() const
  {
    return key_;
  }
  
  void
  storeable::key(const std::string & k)
  {
    key_ = k;
  }
  
  void
  storeable::column(const std::string & name)
  {
    std::ostringstream os;
    os << clazz() << '.' << name;
    column_set_.insert( os.str() );
  }

  const storeable::column_set_t &
  storeable::column_set() const
  {
    return column_set_;
  }

  const storeable::data_t &
  storeable::property_cref(const std::string & name) const
  {
    static const data_t empty;
    auto it = properties_.find(name);
    if( it != properties_.end() )
    {
      return it->second;
    }
    else
    {
      return empty;
    }
  }

  storeable::data_t
  storeable::property(const std::string & name) const
  {
    data_t ret;
    auto it = properties_.find(name);
    if( it != properties_.end() )
    {
      ret = it->second;
    }
    return ret;
  }

  void
  storeable::property(const std::string & name,
                      const char * ptr,
                      size_t len)
  {
    auto it = properties_.find(name);
    if( it == properties_.end() )
    {
      if( !ptr || !len )
        properties_[name] = data_t();
      else
        properties_[name] = data_t(ptr, ptr+len);
      column(name);
    }
    else
    {
      if( !ptr || !len )
        it->second = data_t();
      else
        it->second = data_t(ptr, ptr+len);
    }
  }
  
  void
  storeable::property(const std::string & name,
                      const data_t & dta)
  {
    auto it = properties_.find(name);
    if( it == properties_.end() )
    {
      properties_[name] = dta;
      column(name);
    }
    else
    {
      it->second = dta;
    }
  }
  
  void
  storeable::properties(std::function<void(const std::string & _clazz,
                                           const std::string & _key,
                                           const std::string & _family,
                                           const data_t & _data)> fn) const
  {
    for( auto const & it : properties_ )
    {
      std::ostringstream os;
      os << clazz() << '.' << it.first;
      
      fn(clazz(),
         key(),
         os.str(),
         it.second);
    }
  }
  
  void
  storeable::clear()
  {
    properties_.clear();
    column_set_.clear();
    key_.clear();
  }

}}
