#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "srcsys_credential_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  srcsys_credential_server::on_reply(const rep_base_type::req_item & req,
                                     rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  srcsys_credential_server::on_request(const rep_base_type::req_item & req,
                                       rep_base_type::send_rep_handler handler)
  {
  }

  srcsys_credential_server::srcsys_credential_server(server_context::sptr ctx,
                                                     config_client & cfg_client,
                                                     const std::string & name)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&srcsys_credential_server::on_request,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&srcsys_credential_server::on_reply,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::SRCSYS_CRED_MGR)
  
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(name);
      ep_data.set_svctype(pb::ServiceType::SRCSYS_CRED_MGR);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  srcsys_credential_server::~srcsys_credential_server()
  {
  }  
}}
