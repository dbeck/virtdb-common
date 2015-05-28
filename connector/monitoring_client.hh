#pragma once

#include <connector/req_client.hh>
#include <monitoring.pb.h>
#include <memory>

namespace virtdb { namespace connector {
  
  class monitoring_client :
    public req_client<interface::pb::MonitoringRequest,
                      interface::pb::MonitoringReply>
  {
    typedef req_client<interface::pb::MonitoringRequest,
                       interface::pb::MonitoringReply>     req_base_type;
  public:
    typedef std::shared_ptr<monitoring_client> sptr;
    
    monitoring_client(client_context::sptr ctx,
                      endpoint_client & ep_clnt,
                      const std::string & server);
    
    bool report_up_down(const std::string & name,
                        bool is_up,
                        const std::string & reported_by);
    
    virtual ~monitoring_client();
    virtual void cleanup();
  };
}}
