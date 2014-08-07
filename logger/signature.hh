#pragma once

#include "count_items.hh"

namespace virtdb { namespace logger {

  class signature
  {
  public:
    signature(const count_items & ci_) : _ci(&ci_) {};
    
    template <typename T>
    signature & operator<<(const T &) { return *this; }
    
    const count_items & counts() const { return *_ci; }

  private:
    const count_items * _ci;
  };
  
}}
