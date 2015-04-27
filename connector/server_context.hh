#pragma once

#include <memory>

namespace virtdb { namespace connector {
  
  class server_context
  {
    server_context(const server_context &) = delete;
    server_context& operator=(const server_context &) = delete;
    
  public:
    typedef std::shared_ptr<server_context> sptr;
    
    server_context();
    virtual ~server_context();
  };
  
}}
