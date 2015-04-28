#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "cert_store_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  cert_store_server::on_reply_fwd(const rep_base_type::req_item & req,
                              rep_base_type::rep_item_sptr rep)
  {
    LOG_INFO("FORWARDING");
    on_reply(req, rep);
  }
  
  void
  cert_store_server::on_request_fwd(const rep_base_type::req_item & req,
                                    rep_base_type::send_rep_handler handler)
  {
    LOG_INFO("FORWARDING");
    on_request(req, handler);
  }
  
  cert_store_server::cert_store_server(server_context::sptr ctx,
                                       config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&cert_store_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&cert_store_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::CERT_STORE)
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::CERT_STORE);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  cert_store_server::~cert_store_server()
  {
  }
  
}}
