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
                  pb::ServiceType::COLUMN)
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      ep_data.set_svctype(pb::ServiceType::COLUMN);
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  column_server::~column_server()
  {
  }
  
}}
