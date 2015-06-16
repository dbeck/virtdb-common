#include "column_server.hh"
#include "endpoint_client.hh"
#include <util/constants.hh>
#include <svc_config.pb.h>

using namespace virtdb::interface;

namespace virtdb { namespace connector {
  
  column_server::column_server(server_context::sptr ctx,
                               config_client & cfg_client)
  : pub_base_type(ctx,
                  cfg_client,
                  pb::ServiceType::COLUMN),
    ctx_{ctx}
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      ep_data.set_svctype(pb::ServiceType::COLUMN);
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(util::DEFAULT_ENDPOINT_EXPIRY_MS);
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  column_server::~column_server()
  {
  }
  
  void
  column_server::publish(const std::string & channel,
                         pub_base_type::pub_item_sptr item_sptr)
  {
    pub_base_type::publish(channel, item_sptr);
    ctx_->increase_stat("Column message");
  }

}}
