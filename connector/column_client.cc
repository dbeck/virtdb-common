#include "column_client.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  column_client::column_client(endpoint_client & ep_client,
                               const std::string & server_name)
  : sub_base_type(ep_client,
                  server_name)
  {
  }
  
  column_client::~column_client()
  {
  }
  
  void
  column_client::cleanup()
  {
    sub_base_type::cleanup();
  }

  void
  column_client::rethrow_error()
  {
    sub_base_type::rethrow_error();
  }
  
}}
