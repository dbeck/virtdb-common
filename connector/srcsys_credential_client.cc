#include "srcsys_credential_client.hh"

namespace virtdb { namespace connector {
  
  srcsys_credential_client::srcsys_credential_client(client_context::sptr ctx,
                                                     endpoint_client & ep_clnt,
                                                     const std::string & server)
  : req_base_type(ctx,
                  ep_clnt,
                  server)
  {
  }
  
  srcsys_credential_client::~srcsys_credential_client()
  {
  }
  
  void
  srcsys_credential_client::cleanup()
  {
    req_base_type::cleanup();
  }
  
  bool
  srcsys_credential_client::set_template(const std::string & src_system,
                                         const name_type_map & name_types)
  {
    interface::pb::SourceSystemCredentialRequest req;
    {
      req.set_type(interface::pb::SourceSystemCredentialRequest::SET_TEMPLATE);
      auto * inner = req.mutable_settmpl();
      inner->set_sourcesysname(src_system);
      for( auto it : name_types )
      {
        auto * tmpl = inner->add_templates();
        tmpl->set_name(it.first);
        tmpl->set_type(it.second);
      }
    }
    
    interface::pb::SourceSystemCredentialReply rep;
    
    auto proc_rep = [&](const interface::pb::SourceSystemCredentialReply & r) {
      rep.MergeFrom(r);
      return true;
    };
    
    auto res = this->send_request(req, proc_rep, 10000);
    
    bool ret = res && !rep.has_err() && (rep.type() == interface::pb::SourceSystemCredentialReply::SET_TEMPLATE);
    if( !ret )
    {
      std::string msg;
      if( rep.has_err() )
      {
        auto err = rep.err();
        msg = err.msg();
      }
      LOG_ERROR("failed to send template" << V_(msg) << V_((int)rep.type()) << V_(res) )
    }
    
    return ret;
  }
  
  bool
  srcsys_credential_client::get_credential(const std::string & srcsys_token,
                                           const std::string & srcsys_name,
                                           interface::pb::SourceSystemCredentialReply::GetCredential & cred,
                                           unsigned long timeout_ms)
  {
    using namespace virtdb::interface;
    
    if( srcsys_token.empty() )
    {
      LOG_ERROR("empty srcsys token");
      return false;
    }
    
    if( srcsys_name.empty() )
    {
      LOG_ERROR("empty srcsys_name");
      return false;
    }
    
    std::string itok{srcsys_token};
    std::string sname{srcsys_name};
        
    pb::SourceSystemCredentialRequest req;
    
    {
      req.set_type(pb::SourceSystemCredentialRequest::GET_CREDENTIAL);
      auto * inner = req.mutable_getcred();
      inner->set_sourcesysname(sname);
      inner->set_sourcesystoken(itok);
    }
    
    pb::SourceSystemCredentialReply rep;
    
    auto proc_rep = [&](const pb::SourceSystemCredentialReply & r) {
      rep.MergeFrom(r);
      return true;
    };
    
    bool ret = send_request(req, proc_rep, timeout_ms);
    if( ret && !rep.has_err() && rep.has_getcred() )
    {
      auto & rep_cred = rep.getcred();
      cred.CopyFrom(rep_cred);
      return true;
    }
    
    if( rep.has_err() )
    {
      LOG_ERROR("failed to get source system credential" << V_(sname) << V_(rep.err().msg()));
    }
    
    return false;
  }
  
}}
