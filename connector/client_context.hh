#pragma once

#include <memory>

namespace virtdb { namespace connector {
  
  class client_context
  {
    client_context(const client_context &) = delete;
    client_context& operator=(const client_context &) = delete;
    
  public:
    typedef std::shared_ptr<client_context> sptr;
    
    client_context();
    virtual ~client_context();
  };
  
}}
