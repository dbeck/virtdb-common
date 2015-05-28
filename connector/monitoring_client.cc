#include <connector/monitoring_client.hh>

namespace virtdb { namespace connector {
  
  monitoring_client::monitoring_client(client_context::sptr ctx,
                                       endpoint_client & ep_clnt,
                                       const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  monitoring_client::~monitoring_client()
  {
  }
  
  void
  monitoring_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
  bool
  monitoring_client::report_up_down(const std::string & name,
                                    bool is_up,
                                    const std::string & reported_by)
  {
    using namespace virtdb::interface::pb;
    MonitoringRequest req;
    req.set_type(MonitoringRequest::COMPONENT_ERROR);
    auto inner = req.mutable_comperr();
    inner->set_type(is_up ? (MonitoringRequest::ComponentError::CLEAR) : (MonitoringRequest::ComponentError::DOWN));
    inner->set_impactedpeer(name);
    inner->set_reportedby(reported_by);
    
    return send_request(req,
                        [](const MonitoringReply & rep) { return true; },
                        util::DEFAULT_TIMEOUT_MS);
    
  }
  
}}
