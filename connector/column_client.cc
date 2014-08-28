#include "column_client.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  column_client::column_client(endpoint_client & ep_client,
                               const std::string & provider_name)
  : provider_(provider_name),
    zmqctx_(1),
    column_sub_socket_(zmqctx_, ZMQ_SUB),
    worker_(std::bind(&column_client::worker_function,this))
  {
    // TODO
    worker_.start();
  }
  
  bool
  column_client::worker_function()
  {
    // TODO
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return false;
  }
  
  void
  column_client::watch(const column_location & location,
                       column_monitor mon)
  {
    // TODO
  }
  
  void
  column_client::remove_watches()
  {
    // TODO
  }
  
  void
  column_client::remove_watch(const column_location & location)
  {
    // TODO
  }
  
  bool
  column_client::on_endpoint_data(const interface::pb::EndpointData & ep)
  {
    // TODO
    return false;
  }

  column_client::~column_client()
  {
    worker_.stop();
  }
  
  const std::string &
  column_client::provider_name() const
  {
    return provider_;
  }
}}
