#include "client_context.hh"

namespace virtdb { namespace connector {
  
  client_context::client_context() {}
  
  client_context::~client_context() {}
  
  client_context::client_context(const std::string & epname)
  : name_{epname}
  {
  }
  
  std::string
  client_context::name() const
  {
    std::string ret{name_};
    return ret;
  }
  
  void
  client_context::name(const std::string & epname)
  {
    name_ = epname;
  }

}}
