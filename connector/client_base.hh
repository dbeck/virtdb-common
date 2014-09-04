#pragma once

#include "endpoint_client.hh"

namespace virtdb { namespace connector {

  class client_base
  {
    std::string server_;
    
  public:
    client_base(endpoint_client & ep_clnt,
                const std::string & srv);
    virtual ~client_base();
    
    virtual const std::string & server() const;
    
  private:
    client_base() = delete;
    client_base(const client_base &) = delete;
    client_base & operator=(const client_base &) = delete;
  };
  
}}
