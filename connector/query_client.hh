#pragma once

#include <connector/push_client.hh>
#include <data.pb.h>

namespace virtdb { namespace connector {
  
  class query_client :
      public push_client<interface::pb::Query>
  {
    typedef push_client<interface::pb::Query> push_base_type;
    
  public:
    typedef std::shared_ptr<query_client> sptr;
    
    query_client(client_context::sptr ctx,
                 endpoint_client & ep_clnt,
                 const std::string & server);
    
    virtual ~query_client();
    
    void cleanup();
  };
}}