#pragma once

namespace virtdb { namespace logger {
  
  struct variable_base {};

  template <typename T>
  struct variable : public variable_base
  {
    const T     * val_;
    const char  * name_;

    variable(const char * _name, const T& _val)
    : val_(&_val),
      name_(_name)
    {
    }
  };
  
  template <typename T>
  variable<T> make_variable(const char * _name, const T& _val)
  {
    return variable<T>(_name, _val);
  }
  
}}
