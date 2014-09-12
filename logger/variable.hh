#pragma once

namespace virtdb { namespace logger {
  
  struct variable_base {};

  template <typename T>
  struct variable : public variable_base
  {
    typedef T     type;
    const T     * val_;
    const char  * name_;

    variable(const char * _name, const T& _val)
    : val_(&_val),
      name_(_name)
    {
    }
  };

  template <>
  struct variable<::google::protobuf::Message> : public variable_base
  {
    typedef std::string  type;
    std::string          str_;
    const std::string  * val_;
    const char         * name_;
    
    variable(const char * _name, const ::google::protobuf::Message & _val)
    : val_(&str_), 
      name_(_name)
    {
      str_ = _val.DebugString();
      if( str_.empty() )
        return;

      size_t l = str_.size();
      for( size_t i=0;i<l-1;++i )
      {
        if( (str_[i] == '{' || str_[i] == '}') && str_[i+1] == '\n' )
          str_[++i] = ' ';
      }
    }
  };
  
  template <typename T>
  variable<T> make_variable(const char * _name, const T& _val)
  {
    return variable<T>(_name, _val);
  }
  
}}
