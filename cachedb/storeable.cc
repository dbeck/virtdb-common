#include "storeable.hh"
#include <sstream>

namespace virtdb { namespace cachedb {
  
  storeable::storeable() {}
  storeable::~storeable() {}
  
  void
  storeable::property(const std::string & name,
                      void * ptr,
                      size_t len)
  {
    data itm { ptr, len };
    auto it = properties_.find(name);
    if( it == properties_.end() )
    {
      properties_[name] = itm;
      column(name);
    }
    else
    {
      it->second.buffer_ = ptr;
      it->second.len_    = len;
    }
  }
  
  void
  storeable::properties(std::function<void(const std::string & _clazz,
                                           const std::string & _key,
                                           const std::string & _name,
                                           data _data)> fn)
  {
    for( auto const & it : properties_ )
    {
      fn(clazz(),
         key(),
         it.first,
         it.second);
    }
  }
  
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

  storeable::data
  storeable::property(const std::string name) const
  {
    data ret { 0, 0 };
    auto it = properties_.find(name);
    if( it != properties_.end() )
    {
      ret.buffer_  = it->second.buffer_;
      ret.len_     = it->second.len_;
    }
    return ret;
  }

}}
