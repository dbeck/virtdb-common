#include "query_server.hh"
#include <functional>

using namespace virtdb::interface;

namespace virtdb { namespace connector {
  
  query_server::query_server(config_client & cfg_client)
  : base_type(cfg_client,
              std::bind(&query_server::handler_function,
                        this,
                        std::placeholders::_1))
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      auto conn = ep_data.add_connections();
      conn->MergeFrom(base_type::bound_to());
    }
    cfg_client.get_endpoint_client().register_endpoint(ep_data);
  }
  
  void
  query_server::handler_function(query_sptr qsptr)
  {
  }
  
  void
  query_server::watch(const column_location & location,
                      query_monitor mon)
  {
    // TODO
  }
  
  void
  query_server::remove_watches()
  {
    // TODO
  }
  
  void
  query_server::remove_watch(const column_location & location)
  {
    // TODO
  }
  
  query_server::~query_server()
  {
  }

}}
