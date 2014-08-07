#pragma once

#include "end_msg.hh"
#include "traits.hh"

namespace virtdb { namespace logger {

  class count_items
  {
    size_t items_;
    size_t symbols_;
    size_t variables_;
    size_t data_;
    size_t supported_;
    
  public:
    count_items() : items_(0), symbols_(0), variables_(0), data_(0), supported_(0) {}
    
    count_items & operator<<(const end_msg &) { return *this; }
    
    template <typename T>
    count_items & operator<<(const T &)
    {
      ++items_;
      /*
      symbols_    += traits<T>::is_symbol_;
      variables_  += traits<T>::is_variable_;
      data_       += traits<T>::is_data_;
      supported_  += traits<T>::is_supported_;
       */
      return *this;
    }
    
    template <typename T>
    count_items & operator<<(const T *)
    {
      /*
      ++items_;
      symbols_    += traits<const T *>::is_symbol_;
      variables_  += traits<const T *>::is_variable_;
      data_       += traits<const T *>::is_data_;
      supported_  += traits<const T *>::is_supported_;
       */
      return *this;
    }
    
    size_t items()      const { return items_;     }
    size_t symbols()    const { return symbols_;   }
    size_t variables()  const { return variables_; }
    size_t data()       const { return data_;      }
    size_t supported()  const { return supported_; }
    
  }; // count_items
  
} }
