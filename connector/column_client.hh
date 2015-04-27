#pragma once

#include "endpoint_client.hh"
#include "sub_client.hh"
#include <data.pb.h>
#include <util/zmq_utils.hh>
#include <functional>
#include <vector>
#include <map>

namespace virtdb { namespace connector {
  
  class column_client final :
    public sub_client<interface::pb::Column>
  {
  public:
    typedef sub_client<interface::pb::Column> sub_base_type;
    
  public:
    column_client(client_context::sptr ctx,
                  endpoint_client & ep_client,
                  const std::string & server_name);
    ~column_client();
    
    void cleanup();
    void rethrow_error();
  };
}}
