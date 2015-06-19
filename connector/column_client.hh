#pragma once

#include <connector/endpoint_client.hh>
#include <connector/sub_client.hh>
#include <data.pb.h>

namespace virtdb { namespace connector {
  
  class column_client :
    public sub_client<interface::pb::Column>
  {
  public:
    typedef sub_client<interface::pb::Column> sub_base_type;
    
  public:
    typedef std::shared_ptr<column_client> sptr;
    
    column_client(client_context::sptr ctx,
                  endpoint_client & ep_client,
                  const std::string & server_name);
    virtual ~column_client();
    
    void cleanup();
    void rethrow_error();
  };
}}
