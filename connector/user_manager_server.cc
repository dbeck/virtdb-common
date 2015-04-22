#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "user_manager_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  user_manager_server::on_reply(const rep_base_type::req_item & req,
                                rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  user_manager_server::on_request(const rep_base_type::req_item & req,
                                  rep_base_type::send_rep_handler handler)
  {
  }
  
  user_manager_server::user_manager_server(config_client & cfg_client,
                                           const std::string & name)
  : rep_base_type(cfg_client,
                  std::bind(&user_manager_server::on_request,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&user_manager_server::on_reply,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::USER_MGR)
  
  {
  }
  
  user_manager_server::~user_manager_server()
  {
  }  
}}