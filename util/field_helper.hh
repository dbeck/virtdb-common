#pragma once

#include <meta_data.pb.h>
#include <util/value_type.hh>
#include <map>
#include <string>

namespace virtdb { namespace util {
  
  class field_helper
  {
    // properties are owned by the object we are helping
    typedef std::map<std::string, interface::pb::ValueType> propery_map;
    
    propery_map properties_;
    
  public:
    field_helper();
    virtual ~field_helper();
    
    void init(const interface::pb::Field & field);
    void save(interface::pb::Field & field) const;
    
    template <typename T>
    T get_property(const std::string & name, const T & default_value)
    {
      auto it = properties_.find(name);
      if( it != properties_.end() )
      {
        return util::value_type<T>::get(it->second, 0, default_value);
      }
      else
      {
        return default_value;
      }
    }
    
    template <typename T>
    void set_property(const std::string & name,
             const T & value)
    {
      auto it = properties_.find(name);
      if( it == properties_.end() )
      {
        interface::pb::ValueType vt;
        util::value_type<T>::set(vt, value);
        properties_[name] = vt;
      }
      else
      {
        it->second.Clear();
        util::value_type<T>::set(it->second, value);
      }
    }
    
    void remove_property(const std::string & name);
  };
  
}}
