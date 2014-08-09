#pragma once

#include "count_items.hh"
#include "end_msg.hh"
#include <iostream>

namespace virtdb { namespace logger {

  class signature
  {
  public:
    class part
    {
    public:
      template <typename T>
      part(signature *, const T &)
      {
        // TODO
        std::cout << "FIRST part\n";
      }
      
      template <typename T>
      part(part *, const T &)
      {
        // TODO
        std::cout << "OTHER part\n";
      }

      part(part *, const end_msg &)
      {
        // TODO
        std::cout << "END part\n";
      }
      
      template <typename T>
      part
      operator<<(const T & val) { return part(this, val); }

    };
    
    signature(const count_items & ci_) : _ci(&ci_) {};
    
    // TODO : generate LogParts here
    template <typename T>
    part
    operator<<(const T & val) { return part(this, val); }
    
    const count_items &
    counts() const
    {
      return *_ci;
    }

  private:
    const count_items * _ci;
  };
  
}}
