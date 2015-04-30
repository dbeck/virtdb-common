#pragma once

#include <connector/config_client.hh>
#include <connector/pub_server.hh>
#include <data.pb.h>

namespace virtdb { namespace connector {
  
  class column_server final :
      public pub_server<interface::pb::Column>
  {
    typedef pub_server<interface::pb::Column>  pub_base_type;
    
  public:
    column_server(server_context::sptr ctx,
                  config_client & cfg_client);
    virtual ~column_server();
  };
}}
