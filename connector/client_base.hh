#pragma once

#include "endpoint_client.hh"
#include <connector/client_context.hh>

namespace virtdb { namespace connector {

  class client_base
  {
    client_context::sptr  context_;
    std::string           server_;
    
  public:
    client_base(client_context::sptr ctx,
                endpoint_client & ep_clnt,
                const std::string & srv);
    virtual ~client_base();
    
    virtual const std::string & server() const;
    
  private:
    client_base() = delete;
    client_base(const client_base &) = delete;
    client_base & operator=(const client_base &) = delete;
  };
  
}}
