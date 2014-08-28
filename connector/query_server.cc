#include "query_server.hh"
#include <functional>

namespace virtdb { namespace connector {
  
  query_server::query_server(config_client & cfg_client)
  : zmqctx_(1),
    query_pull_socket_(zmqctx_, ZMQ_PULL),
    worker_(std::bind(&query_server::worker_function,this)),
    // TODO : make number of monitor threads configurable ???
    query_monitor_queue_(1,std::bind(&query_server::handler_function,
                                     this,
                                     std::placeholders::_1))
  {
    // TODO : bind ports
    
    // start worker before registering endpoints
    worker_.start();
    
    // TODO : register endpoints
  }
  
  bool
  query_server::worker_function()
  {
    // TODO
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return false;
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
    worker_.stop();
  }

}}
