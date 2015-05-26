#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "monitoring_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  monitoring_server::on_reply_fwd(const rep_base_type::req_item & req,
                                         rep_base_type::rep_item_sptr rep)
  {
    on_reply(req, rep);
  }
  
  void
  monitoring_server::on_request_fwd(const rep_base_type::req_item & req,
                                           rep_base_type::send_rep_handler handler)
  {
    on_request(req, handler);
  }
  
  void
  monitoring_server::on_reply(const rep_base_type::req_item & req,
                                     rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  monitoring_server::on_request(const rep_base_type::req_item & req,
                                       rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr rep{new rep_base_type::rep_item};
    
    try
    {
      switch( req.type() )
      {
          /*
           REPORT_STATS       = 2;
           SET_STATE          = 3;
           COMPONENT_ERROR    = 4;
           REQUEST_ERROR      = 5;
           GET_STATES         = 6;
           
           required   RequestType      Type      = 1;
           optional   ReportStats      RepStats  = 2;
           optional   SetState         SetSt     = 3;
           optional   ComponentError   CompErr   = 4;
           optional   RequestError     ReqErr    = 5;
           optional   GetStates        GetSts    = 6;
           
           
           message NamedStat {
           required   string   Name   = 1;
           required   double   Stat   = 2;
           }
           message ReportStats {
           required   string      Name    = 1;
           repeated   NamedStat   Stats   = 2;
           }
           message SetState {
           enum Types {
           NOT_INITIALIZED   = 1;
           COMPONENT_ERROR   = 2;
           CLEAR             = 3;
           }
           required   string   Name   = 1;
           required   Types    Type   = 2;
           optional   string   Msg    = 3;
           }
           message ComponentError {
           enum Types {
           DOWN              = 1;  // by EndpointService
           TIMED_OUT         = 2;  // by Components
           UPSTREAM_ERROR    = 3;  //   -||-
           CLEAR             = 4;  //   -||-
           }
           required   string   ReportedBy     = 1;
           required   string   ImpactedPeer   = 2;
           required   Types    Type           = 3;
           optional   string   Message        = 4;
           }
           message RequestError {
           enum Types {
           INVALID_REQUEST    = 1;
           UPSTREAM_ERROR     = 2;
           UPSTREAM_TIMEOUT   = 3;
           RESOURCE_LIMIT     = 4;
           }
           required   string   ReportedBy     = 1;
           required   string   ImpactedPeer   = 2;
           required   Types    Type           = 3;
           optional   string   RequestId      = 4;
           optional   string   Table          = 5;
           optional   string   Schema         = 6;
           optional   string   Message        = 7;
           }
           message GetStates {
           optional   string   Name   = 1;
           }

           
           */
        case pb::MonitoringRequest::REPORT_STATS:
        {
          if( !req.has_repstats() )             { THROW_("missing ReportStats from MonitoringRequest"); }
          auto const & req_msg = req.repstats();
          
          rep->set_type(pb::MonitoringReply::REPORT_STATS);
          break;
        }
        case pb::MonitoringRequest::SET_STATE:
        {
          if( !req.has_setst() )             { THROW_("missing SetState from MonitoringRequest"); }
          auto const & req_msg = req.setst();
          rep->set_type(pb::MonitoringReply::SET_STATE);
          break;
        }
        case pb::MonitoringRequest::COMPONENT_ERROR:
        {
          if( !req.has_comperr() )             { THROW_("missing ComponentError from MonitoringRequest"); }
          auto const & req_msg = req.comperr();
          rep->set_type(pb::MonitoringReply::COMPONENT_ERROR);
          break;
        }
        case pb::MonitoringRequest::REQUEST_ERROR:
        {
          if( !req.has_reqerr() )             { THROW_("missing RequestError from MonitoringRequest"); }
          auto const & req_msg = req.reqerr();
          rep->set_type(pb::MonitoringReply::REQUEST_ERROR);
          break;
        }
        case pb::MonitoringRequest::GET_STATES:
        {
          if( !req.has_getsts() )             { THROW_("missing GetStates from MonitoringRequest"); }
          auto const & req_msg = req.getsts();
          rep->set_type(pb::MonitoringReply::GET_STATES);
          break;
        }
        default:
        {
          THROW_("Unkown request type");
          break;
        }
      };
    }
    catch (const std::exception & e)
    {
      rep->Clear();
      rep->set_type(pb::MonitoringReply::ERROR_MSG);
      auto * err = rep->mutable_err();
      err->set_msg(e.what());
      LOG_ERROR(E_(e));
    }
    
    // send a single reply back
    bool has_more = false;
    handler(rep, has_more);
  }
  
  monitoring_server::monitoring_server(server_context::sptr ctx,
                                       config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&monitoring_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&monitoring_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::MONITORING)
  
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::MONITORING);
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  monitoring_server::~monitoring_server()
  {
  }  
}}
