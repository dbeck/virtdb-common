#include "query_server.hh"
#include <functional>

namespace virtdb { namespace connector {
  
  query_server::query_server(config_client & cfg_client)
  : base_type(cfg_client,
              std::bind(&query_server::handler_function,
                        this,
                        std::placeholders::_1))
  {
    // TODO : register endpoints
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
