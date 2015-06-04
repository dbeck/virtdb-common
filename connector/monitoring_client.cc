#include <connector/monitoring_client.hh>

namespace virtdb { namespace connector {
  
  monitoring_client::sptr monitoring_client::global_instance_;
  
  void
  monitoring_client::set_global_instance(sptr s)
  {
    global_instance_ = s;
  }
  
  monitoring_client::sptr
  monitoring_client::global_instance()
  {
    return global_instance_;
  }
  
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
    inner->set_type(is_up ?
                    (MonitoringRequest::ComponentError::CLEAR) :
                    (MonitoringRequest::ComponentError::DOWN));
    inner->set_impactedpeer(name);
    inner->set_reportedby(reported_by);
    
    return send_request(req,
                        [](const MonitoringReply & rep) { return true; },
                        util::DEFAULT_TIMEOUT_MS);
  }
  
  bool
  monitoring_client::report_state(const std::string & name,
                                  interface::pb::MonitoringRequest_SetState_Types t,
                                  const char * msg)
  {
    using namespace virtdb::interface::pb;
    MonitoringRequest req;
    req.set_type(MonitoringRequest::SET_STATE);
    auto inner = req.mutable_setst();
    inner->set_name(name);
    inner->set_type(t);
    if( msg ) inner->set_msg(msg);
    
    return send_request(req,
                        [](const MonitoringReply & rep) { return true; },
                        util::DEFAULT_TIMEOUT_MS);
  }
  
  bool
  monitoring_client::report_bad_table_request(const std::string & name,
                                              interface::pb::MonitoringRequest_RequestError_Types t,
                                              const std::string & request_id,
                                              const std::string & table,
                                              const char * schema,
                                              const char * msg)
  {
    using namespace virtdb::interface::pb;
    MonitoringRequest req;
    req.set_type(MonitoringRequest::REQUEST_ERROR);
    auto inner = req.mutable_reqerr();
    inner->set_reportedby(name);
    inner->set_impactedpeer(name);
    inner->set_type(t);
    inner->set_requestid(request_id);
    inner->set_table(table);
    
    if( schema )  inner->set_schema(schema);
    if( msg )     inner->set_message(msg);
    
    return send_request(req,
                        [](const MonitoringReply & rep) { return true; },
                        util::DEFAULT_TIMEOUT_MS);
  }
  
  bool
  monitoring_client::report_upstream_error(const std::string & impacted_peer,
                                           const std::string & reported_by,
                                           const char * msg,
                                           bool clear)
  {
    using namespace virtdb::interface::pb;
    MonitoringRequest req;
    req.set_type(MonitoringRequest::COMPONENT_ERROR);
    auto inner = req.mutable_comperr();
    
    if( clear )
      inner->set_type(MonitoringRequest::ComponentError::UPSTREAM_ERROR);
    else
      inner->set_type(MonitoringRequest::ComponentError::CLEAR);
    
    inner->set_impactedpeer(impacted_peer);
    inner->set_reportedby(reported_by);
    if( msg ) inner->set_message(msg);
    
    return send_request(req,
                        [](const MonitoringReply & rep) { return true; },
                        util::DEFAULT_TIMEOUT_MS);
  }
  
}}
