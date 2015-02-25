#include "storeable.hh"
#include <sstream>
#include <ctime>
#include <iomanip>
#include <locale>

namespace virtdb { namespace cachedb {
    
  bool
  storeable::convert(const data_t & in,
                     std::chrono::system_clock::time_point & out)
  {
    if( in.empty() )
    {
      return false;
    }
    std::istringstream is{in};
    std::tm tm;
    
    is >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S %z");
    
    tm.tm_isdst   = -1;
    tm.tm_gmtoff  = 0;
    tm.tm_zone    = nullptr;
    
    std::time_t t = std::mktime(&tm);
    if( (int)t == -1 )
    {
      return false;
    }
    out = std::chrono::system_clock::from_time_t(t);
    return true;
  }
  
  bool
  storeable::convert(const std::chrono::system_clock::time_point & in,
                     data_t & out)
  {
    std::time_t t = std::chrono::system_clock::to_time_t(in);
    std::tm tm;
    auto * ret = localtime_r(&t, &tm);
    
    tm.tm_isdst   = -1;
    tm.tm_gmtoff  = 0;
    tm.tm_zone    = nullptr;
    
    std::time_t t2 = std::mktime(&tm);
    
    if( !ret )
    {
      return false;
    }
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %z");
    out = os.str();
    return true;
  }
  
  bool
  storeable::convert(const data_t & in,
                     size_t & out)
  {
    if( in.empty() )
    {
      return false;
    }
    auto res = atoll(in.c_str());
    if( res < 0 )
    {
      return false;
    }
    out = res;
    return true;
  }
  
  bool
  storeable::convert(size_t in,
                     data_t & out)
  {
    out = std::to_string(in);
    return true;
  }
  
  bool
  storeable::convert(const data_t & in,
                     int64_t & out)
  {
    if( in.empty() )
    {
      return false;
    }
    out = atoll(in.c_str());
    return true;
  }
  
  bool
  storeable::convert(int64_t in,
                     data_t & out)
  {
    out = std::to_string(in);
    return true;
  }
  
  ///
  
  storeable::qual_name::qual_name(const std::string & n) : name_{n} {}
  
  storeable::qual_name::qual_name(const storeable & st,
                                  const std::string & n)
  {
    std::ostringstream os;
    os << st.clazz() << '.' << n;
    name_ = os.str();
  }
  
  storeable::qual_name::qual_name(const std::string & clazz,
                                  const std::string & n)
  {
    std::ostringstream os;
    os << clazz << '.' << n;
    name_ = os.str();
  }

  //
  
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
  
  const storeable::qual_name &
  storeable::column(const std::string & name)
  {
    std::ostringstream os;
    os << clazz() << '.' << name;
    qual_name nm{os.str()};
    auto ret = column_set_.insert(nm);
    return *(ret.first);
  }
  
  void
  storeable::column(const qual_name & name)
  {
    column_set_.insert(name);
  }

  const storeable::column_set_t &
  storeable::column_set() const
  {
    return column_set_;
  }
  
  const storeable::data_t &
  storeable::property_cref(const qual_name & name) const
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
  
  storeable::data_t &
  storeable::property_ref(const qual_name & name)
  {
    auto it = properties_.find(name);
    if( it != properties_.end() )
    {
      return it->second;
    }
    else
    {
      column(name);
      auto res = properties_.insert(std::make_pair(name,std::string()));
      return (res.first->second);
    }
  }

  storeable::data_t
  storeable::property(const qual_name & name) const
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
  storeable::property(const qual_name & name,
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
  storeable::property(const qual_name & name,
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
                                           const qual_name & _name,
                                           const data_t & _data)> fn) const
  {
    for( auto const & it : properties_ )
    {
      fn(clazz(),
         key(),
         it.first,
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
