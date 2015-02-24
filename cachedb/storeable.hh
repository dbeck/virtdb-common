#pragma once

#include <string>
#include <set>
#include <map>
#include <functional>
#include <chrono>

namespace virtdb { namespace cachedb {
  
  class storeable
  {
  public:
    struct qual_name
    {
      // clazz + '.' + name;
      std::string name_;

      inline bool operator<(const qual_name & o) const { return name_ < o.name_; }
      inline bool operator==(const qual_name & o) const { return name_ == o.name_; }
      
      // clazz + '.' + name;
      qual_name(const std::string & n);
      
      // constructs from st.clazz + '.' + name
      qual_name(const storeable & st, const std::string name);
    };

    typedef std::string                  data_t;
    typedef std::set<qual_name>          column_set_t;
    typedef std::map<qual_name, data_t>  properties_t;
    
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
    virtual const qual_name & column(const std::string & name);
    virtual void column(const qual_name & name);
    virtual const column_set_t & column_set() const;
    
    virtual const data_t & property_cref(const qual_name & name) const;
    virtual data_t & property_ref(const qual_name & name);
    
    virtual data_t property(const qual_name & name) const;
    virtual void property(const qual_name & name,
                          const char * ptr,
                          size_t len);
    virtual void property(const qual_name & name,
                          const data_t & dta);
    virtual void properties(std::function<void(const std::string & _clazz,
                                               const std::string & _key,
                                               const qual_name & _name,
                                               const data_t & _data)>) const;
    virtual void clear();
    
    storeable();
    virtual ~storeable();
    
    // conversion helpers
    static bool convert(const data_t & in,
                        std::chrono::system_clock::time_point & out);
    
    static bool convert(const std::chrono::system_clock::time_point & in,
                        data_t & out);
    
    static bool convert(const data_t & in,
                        size_t & out);
    
    static bool convert(size_t in,
                        data_t & out);

    static bool convert(const data_t & in,
                        int64_t & out);
    
    static bool convert(int64_t in,
                        data_t & out);

  
  };
  
}}
