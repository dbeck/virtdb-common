#pragma once

#include <connector/req_client.hh>
#include <monitoring.pb.h>

namespace virtdb { namespace connector {
  
  class monitoring_client :
    public req_client<interface::pb::MonitoringRequest,
                      interface::pb::MonitoringReply>
  {
    typedef req_client<interface::pb::MonitoringRequest,
                       interface::pb::MonitoringReply>     req_base_type;
  public:
    monitoring_client(client_context::sptr ctx,
                      endpoint_client & ep_clnt,
                      const std::string & server);
    
    virtual ~monitoring_client();
    virtual void cleanup();
  };
}}
