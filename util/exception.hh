#pragma once

#include <exception>
#include <sstream>
#include <string>

namespace virtdb { namespace util {

  class exception : public std::exception
  {
    // must pass minimal location information as below
    exception() = delete;

  protected:
    std::string data_;
  
  public:
    exception(
      const char * file,
      unsigned int line,
      const char * func,
      const char * msg)
    {
      std::ostringstream os;
      os << '[' << file << ':' << line << "] " << func << "() ";
      if( msg ) os << "msg=" << msg << " ";
      data_ = os.str();
    }

    virtual ~exception() {}

    const char* what() const noexcept(true)
    {
      return data_.c_str();
    }
  };

} }

#ifndef THROW_
#define THROW_(MSG) throw virtdb::util::exception(__FILE__,__LINE__,__func__,MSG);
#endif // THROW_

