#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "srcsys_credential_server.hh"

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  srcsys_credential_server::on_reply_fwd(const rep_base_type::req_item & req,
                                         rep_base_type::rep_item_sptr rep)
  {
    on_reply(req, rep);
  }
  
  void
  srcsys_credential_server::on_request_fwd(const rep_base_type::req_item & req,
                                           rep_base_type::send_rep_handler handler)
  {
    on_request(req, handler);
  }

  void
  srcsys_credential_server::on_reply(const rep_base_type::req_item & req,
                                     rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  srcsys_credential_server::on_request(const rep_base_type::req_item & req,
                                       rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr rep{new rep_base_type::rep_item};
    
    try
    {
      switch( req.type() )
      {
        case pb::SourceSystemCredentialRequest::SET_CREDENTIAL:
        {
          if( !req.has_setcred() )             { THROW_("missing SetCredential from SourceSystemCredentialRequest"); }
          auto const & setcr = req.setcred();
          if( !setcr.has_sourcesysname() )     { THROW_("missing SourceSysName from SetCredential"); }
          if( !setcr.has_sourcesystoken() )    { THROW_("missing SourceSysToken from SetCredential"); }
          if( !setcr.has_creds() )             { THROW_("missing CredentialValues from SetCredential"); }
          auto const & crvals = setcr.creds();
          if( !crvals.namedvalues_size() )     { THROW_("empty CredentialValues"); }
          
          cred_sptr crptr{new pb::CredentialValues{crvals}};
          name_token nt{setcr.sourcesysname(), setcr.sourcesystoken()};

          {
            lock l(mtx_);
            // TODO : validate Token ???
            credentials_[nt] = crptr;
          }
          
          rep->set_type(pb::SourceSystemCredentialReply::SET_CREDENTIAL);
          break;
        }
          
        case pb::SourceSystemCredentialRequest::GET_CREDENTIAL:
        {
          if( !req.has_getcred() ) { THROW_("missing GetCredential from SourceSystemCredentialRequest"); }
          auto const & getcr = req.getcred();
          if( !getcr.has_sourcesysname() )  { THROW_("missing SourceSysName from GetCredential"); }
          if( !getcr.has_sourcesystoken() ) { THROW_("missing SourceSysToken from GetCredential"); }
          
          name_token nt{getcr.sourcesysname(), getcr.sourcesystoken()};
          
          {
            lock l(mtx_);
            auto it = credentials_.find(nt);
            if( it == credentials_.end() ) { THROW_("no such source system credential"); }
            auto * repcreds = rep->mutable_getcred();
            auto * tmp = repcreds->mutable_creds();
            tmp->MergeFrom(*(it->second));
          }
          
          rep->set_type(pb::SourceSystemCredentialReply::GET_CREDENTIAL);
          break;
        }
          
        case pb::SourceSystemCredentialRequest::DELETE_CREDENTIAL:
        {
          if( !req.has_delcred() ) { THROW_("missing DeleteCredential from SourceSystemCredentialRequest"); }
          auto const & delcr = req.delcred();
          if( !delcr.has_sourcesysname() )  { THROW_("missing SourceSysName from DeleteCredential"); }
          if( !delcr.has_sourcesystoken() ) { THROW_("missing SourceSysToken from DeleteCredential"); }
          
          name_token nt{delcr.sourcesysname(), delcr.sourcesystoken()};
          
          {
            lock l(mtx_);
            auto it = credentials_.find(nt);
            if( it == credentials_.end() ) { THROW_("no such source system credential"); }
            credentials_.erase(it);
          }
          rep->set_type(pb::SourceSystemCredentialReply::DELETE_CREDENTIAL);
          break;
        }
          
        case pb::SourceSystemCredentialRequest::SET_TEMPLATE:
        {
          if( !req.has_settmpl() ) { THROW_("missing SetTemplate from SourceSystemCredentialRequest"); }
          auto const & settmpl = req.settmpl();
          if( !settmpl.has_sourcesysname() ) { THROW_("missing SourceSysName from SetTemplate"); }
          
          tmpl_sptr val{new pb::SourceSystemCredentialReply::GetTemplate};
          for( auto const & s : settmpl.templates() )
          {
            auto * t = val->add_templates();
            t->MergeFrom(s);
          }
          
          {
            lock l(mtx_);
            // TODO : validate source system by certificate
            templates_[settmpl.sourcesysname()] = val;
          }
          
          rep->set_type(pb::SourceSystemCredentialReply::SET_TEMPLATE);
          break;
        }
          
        case pb::SourceSystemCredentialRequest::GET_TEMPLATE:
        {
          if( !req.has_gettmpl() ) { THROW_("missing GetTemplate from SourceSystemCredentialRequest"); }
          auto const & gettmpl = req.gettmpl();
          if( !gettmpl.has_sourcesysname() ) { THROW_("missing SourceSysName from GetTemplate"); }
          
          {
            lock l(mtx_);
            auto it = templates_.find(gettmpl.sourcesysname());
            if( it == templates_.end() ) { THROW_("no template for source system"); }
            auto * tmp = rep->mutable_gettmpl();
            tmp->MergeFrom(*(it->second));
          }

          rep->set_type(pb::SourceSystemCredentialReply::GET_TEMPLATE);
          break;
        }

        default:
        {
          THROW_("Unkown request type");
          break;
        }
      };
    }
    catch (const std::exception & e)
    {
      rep->set_type(pb::SourceSystemCredentialReply::ERROR_MSG);
      auto * err = rep->mutable_err();
      err->set_msg(e.what());
      LOG_ERROR(E_(e));
    }
    
    // send a single reply back
    bool has_more = false;
    handler(rep, has_more);

  }
  
  srcsys_credential_server::srcsys_credential_server(server_context::sptr ctx,
                                                     config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&srcsys_credential_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&srcsys_credential_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::SRCSYS_CRED_MGR)
  
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::SRCSYS_CRED_MGR);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  srcsys_credential_server::~srcsys_credential_server()
  {
  }  
}}
