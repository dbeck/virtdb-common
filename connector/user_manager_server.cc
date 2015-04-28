#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "user_manager_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  user_manager_server::on_reply_fwd(const rep_base_type::req_item & req,
                                    rep_base_type::rep_item_sptr rep)
  {
    LOG_INFO("FORWARDING");
    on_reply(req, rep);
  }
  
  void
  user_manager_server::on_request_fwd(const rep_base_type::req_item & req,
                                      rep_base_type::send_rep_handler handler)
  {
    LOG_INFO("FORWARDING");
    on_request(req, handler);
  }
  
  user_manager_server::user_manager_server(server_context::sptr ctx,
                                           config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&user_manager_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&user_manager_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::USER_MGR)
  
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::USER_MGR);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  user_manager_server::~user_manager_server()
  {
  }  
}}