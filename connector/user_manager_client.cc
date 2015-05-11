#include "user_manager_client.hh"
#include <future>

namespace virtdb { namespace connector {
  
  user_manager_client::user_manager_client(client_context::sptr ctx,
                                           endpoint_client & ep_clnt,
                                           const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  user_manager_client::~user_manager_client()
  {
  }
  
  void
  user_manager_client::cleanup()
  {
    req_base_type::cleanup();
  }
    
  bool
  user_manager_client::token_is_admin(const std::string & token,
                                      unsigned long timeout_ms)
  {
    interface::pb::UserManagerRequest req;
    interface::pb::UserManagerReply rep;
    auto fun = [&rep](const interface::pb::UserManagerReply & tmp_rep) {
      rep.MergeFrom(tmp_rep);
      return true;
    };
    bool res = this->send_request(req, fun, timeout_ms);
    if( !res ||
        rep.has_err() ||
       rep.type() != rep.LIST_USERS )
    {
      return false;
    }
    auto lsrep = rep.lstusers();
    for( auto const & i : lsrep.users() )
    {
      for( auto const & ii : i.logintokens() )
      {
        if( token == ii )
        {
          return true;
        }
      }
    }
    return false;
  }
  
}}
