#pragma once

#include <memory>
#include <string>

namespace virtdb { namespace connector {
  
  class client_context
  {
    client_context(const client_context &) = delete;
    client_context& operator=(const client_context &) = delete;
    std::string name_;
    
  public:
    typedef std::shared_ptr<client_context> sptr;
    
    client_context();
    client_context(const std::string & epname);    
    const std::string & name() const;
    void name(const std::string & epname);
    virtual ~client_context();
  };
  
}}
