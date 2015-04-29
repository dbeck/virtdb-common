#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "cert_store_server.hh"
#include <logger.hh>
#include <ctime>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  void
  cert_store_server::on_reply_fwd(const rep_base_type::req_item & req,
                              rep_base_type::rep_item_sptr rep)
  {
    on_reply(req, rep);
  }
  
  void
  cert_store_server::on_request_fwd(const rep_base_type::req_item & req,
                                    rep_base_type::send_rep_handler handler)
  {
    on_request(req, handler);
  }
  
  void
  cert_store_server::on_reply(const rep_base_type::req_item & req,
                              rep_base_type::rep_item_sptr rep)
  {
  }
  
  void
  cert_store_server::on_request(const rep_base_type::req_item & req,
                                rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr rep{new rep_base_type::rep_item};
    
    try
    {

      switch( req.type() )
      {
        case pb::CertStoreRequest::CREATE_TEMP_KEY:
        {
          if( !req.has_create() ) { THROW_("missing CreateTempKey from CertStoreRequest"); }
          if( !req.create().has_cert() ) { THROW_("missing Certificate from CreateTempKey"); }
          if( !req.create().cert().has_componentname() ) { THROW_("missing component_name from Certificate"); }
          if( !req.create().cert().has_publickey() ) { THROW_("missing public_key from Certificate"); }

          const pb::Certificate & cert = req.create().cert();
          name_key nk{cert.componentname(), cert.publickey()};
          cert_sptr cert_p{new pb::Certificate{cert}};
          std::string auth_code{std::to_string(time(NULL))};

          {
            lock l(mtx_);
            if( certs_.count(nk) > 0 ) { THROW_("certificate already exists"); }
            
            // TODO : check security here
            // TODO : handle expiry here
            // TODO : handle AUTHCODE here

            cert_p->set_approved(false);
            cert_p->set_requestedatepoch((uint64_t)::time(nullptr));
            certs_[nk] = cert_p;
          }
          
          rep->set_type(pb::CertStoreReply::CREATE_TEMP_KEY);
          rep->mutable_create()->set_authcode(auth_code);
          break;
        }
          
        case pb::CertStoreRequest::APPROVE_TEMP_KEY:
        {
          if( !req.has_approve() ) { THROW_("missing ApproveTempKey from CertStoreRequest"); }
          if( !req.approve().has_cert() ) { THROW_("missing Certificate from ApproveTempKey"); }
          if( !req.approve().cert().has_componentname() ) { THROW_("missing component_name from Certificate"); }
          if( !req.approve().cert().has_publickey() ) { THROW_("missing public_key from Certificate"); }
          
          const pb::Certificate & cert = req.approve().cert();
          name_key nk{cert.componentname(), cert.publickey()};
          cert_sptr cert_p{new pb::Certificate{cert}};
          
          {
            lock l(mtx_);
            
            // TODO : check security here
            // TODO : handle expiry here
            // TODO : handle AUTHCODE here
            
            cert_p->set_approved(true);
            cert_p->set_requestedatepoch((uint64_t)::time(nullptr));
            certs_[nk] = cert_p;
          }
          
          rep->set_type(pb::CertStoreReply::APPROVE_TEMP_KEY);
          break;
        }
          
        case pb::CertStoreRequest::LIST_KEYS:
        {
          if( !req.has_list() ) { THROW_("missing ListKeys from CertStoreRequest"); }
          std::string component;
          if( req.list().has_componentname() )
            component = req.list().componentname();
          
          {
            lock l(mtx_);
            
            auto it   = certs_.begin();
            auto end  = certs_.end();
            
            auto lst = rep->mutable_list();
            
            for( ; it!=end; ++it )
            {
              const pb::Certificate & cert = *(it->second);
              bool skip = false;
              
              if( !component.empty() )
              {
                if( cert.componentname() != component )
                {
                  skip = true;
                }
              }
              
              if( cert.approved() && req.list().approvedkeys() && !skip )
              {
                pb::Certificate * ctmp = lst->add_certs();
                ctmp->MergeFrom(cert);
              }
              else if( !cert.approved() && req.list().tempkeys() && !skip )
              {
                pb::Certificate * ctmp = lst->add_certs();
                ctmp->MergeFrom(cert);
              }
            }
          }
          
          rep->set_type(pb::CertStoreReply::LIST_KEYS);
          break;
        }
  
        case pb::CertStoreRequest::DELETE_KEY:
        {
          if( !req.has_del() ) { THROW_("missing DeleteKey from CertStoreRequest"); }
          if( !req.del().has_cert() ) { THROW_("missing Certificate from CreateTempKey"); }
          if( !req.del().cert().has_componentname() ) { THROW_("missing component_name from Certificate"); }
          if( !req.del().cert().has_publickey() ) { THROW_("missing public_key from Certificate"); }
 
          const pb::Certificate & cert = req.del().cert();
          name_key nk{cert.componentname(), cert.publickey()};

          {
            lock l(mtx_);
            auto it = certs_.find(nk);
            if( it == certs_.end() ) { THROW_("Certificate not found"); }
           
            // TODO : check security here
            certs_.erase(it);
          }
          
          rep->set_type(pb::CertStoreReply::DELETE_KEY);
          break;
        }
          
        default:
          break;
      };
    }
    catch (const std::exception & e)
    {
      rep->set_type(pb::CertStoreReply::ERROR_MSG);
      auto * err = rep->mutable_err();
      err->set_msg(e.what());
      LOG_ERROR(E_(e));
    }
    
    // send a single reply back
    bool has_more = false;
    handler(rep, has_more);
  }
  
  cert_store_server::cert_store_server(server_context::sptr ctx,
                                       config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&cert_store_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&cert_store_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::CERT_STORE)
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::CERT_STORE);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
    }
  }
  
  cert_store_server::~cert_store_server()
  {
  }
  
}}
