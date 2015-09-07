#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "cert_store_server.hh"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <logger.hh>
#include <ctime>
#include <fstream>

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
  
  std::string
  cert_store_server::generate_authcode() const
  {
    static std::atomic<int> counter{0};
    return std::to_string(time(NULL))+"-"+std::to_string(++counter);
  }
  
  void
  cert_store_server::expire_certificate(cert_sptr p)
  {
    lock l(mtx_);
    
    const pb::Certificate & cert = *p;
    name_key nk{cert.componentname(), cert.publickey()};
    
    {
      lock l(mtx_);
      auto it = certs_.find(nk);
      if( it == certs_.end() ) { THROW_("Certificate not found"); }
      
      std::string authcode;
      if( !it->second->has_authcode() )
      {
        authcode = it->second->authcode();
        auto cit = auth_codes_.find(it->second->authcode());
        if( cit == auth_codes_.end() ) { LOG_ERROR("AuthCode for" << M_(*it->second) << "not found in internal map"); }
        else
        {
          auth_codes_.erase(cit);
        }
      }
      certs_.erase(it);
    }
  }
  
  void
  cert_store_server::validate_create_request(cert_sptr)
  {
    // subclasses may add more validation
    // - THROW_(...) if not valid
  }
  
  void
  cert_store_server::validate_approval_request(const std::string & auth_code,
                                               const std::string & login_token,
                                               cert_sptr)
  {
    // subclasses may add more validation
    // - THROW_(...) if not valid
  }
  
  void
  cert_store_server::validate_delete_request(const std::string & login_token,
                                             cert_sptr)
  {
    // subclasses may add more validation
    // - THROW_(...) if not valid
  }
  
  bool
  cert_store_server::allow_authcode_listing() const
  {
    // subclasses may disallow authcodes in LIST responses
    return true;
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
          
          std::string auth_code{generate_authcode()};

          {
            lock l(mtx_);
            if( certs_.count(nk) > 0 ) { THROW_("certificate already exists"); }
            for( auto const & i : certs_ )
            {
              if( i.first.first == cert.componentname() &&
                  i.second->approved() )
              {
                THROW_("approved certificate already exists for this component");
              }
            }
            
            cert_p->set_approved(false);
            cert_p->set_requestedatepoch((uint64_t)::time(nullptr));
            cert_p->set_authcode(auth_code);
            
            // throws exception if validation fails
            validate_create_request(cert_p);
            
            // TODO : handle expiry here

            certs_[nk] = cert_p;
            auth_codes_[auth_code] = nk;
          }
          
          rep->set_type(pb::CertStoreReply::CREATE_TEMP_KEY);
          rep->mutable_create()->set_authcode(auth_code);
          break;
        }
          
        case pb::CertStoreRequest::APPROVE_TEMP_KEY:
        {
          if( !req.has_approve() ) { THROW_("missing ApproveTempKey from CertStoreRequest"); }
          auto const & approve = req.approve();
          if( !approve.has_authcode() ) { THROW_("missing AuthCode from ApproveTempKey"); }
          if( !approve.has_logintoken() ) { THROW_("missing LoginToken from ApproveTempKey"); }
          if( !approve.has_componentname() ) { THROW_("missing ComponentName from ApproveTempKey"); }
          
          {
            lock l(mtx_);
            auto cit = auth_codes_.find(approve.authcode());
            if( cit == auth_codes_.end() ) { THROW_("invalid AuthCode in ApproveTempKey message"); }
            name_key nk{cit->second.first, cit->second.second};
            auto nit = certs_.find(nk);
            if( nit == certs_.end() ) { THROW_("cannot find certificate for AuthCode"); }
            if( approve.componentname() != nit->second->componentname() )
            {
              THROW_("AuthCode is not valid for this component");
            }
            
            // throws exception if validation fails
            validate_approval_request(approve.authcode(),
                                      approve.logintoken(),
                                      nit->second);
            
            // TODO : handle expiry here
            
            nit->second->set_approved(true);
            nit->second->set_requestedatepoch((uint64_t)::time(nullptr));
            auth_codes_.erase(cit);
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
              pb::Certificate cert{*(it->second)};
              bool skip = false;
              
              if( !component.empty() )
              {
                if( cert.componentname() != component )
                {
                  skip = true;
                }
              }
              
              if( !allow_authcode_listing() ) cert.clear_authcode();
              
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
          auto const & del = req.del();
          if( !req.del().has_cert() ) { THROW_("missing Certificate from DeleteKey"); }
          if( !req.del().cert().has_componentname() ) { THROW_("missing component_name from Certificate"); }
          if( !req.del().cert().has_publickey() ) { THROW_("missing public_key from Certificate"); }
 
          const pb::Certificate & cert = req.del().cert();
          name_key nk{cert.componentname(), cert.publickey()};

          {
            lock l(mtx_);
            auto it = certs_.find(nk);
            if( it == certs_.end() ) { THROW_("Certificate not found"); }
            
            // throws exception if validation fails
            validate_delete_request(del.logintoken(),
                                    it->second);
           
            std::string authcode;
            if( !it->second->has_authcode() )
            {
              authcode = it->second->authcode();
              auto cit = auth_codes_.find(authcode);
              if( cit == auth_codes_.end() ) { LOG_ERROR("AuthCode for" << M_(*it->second) << "not found in internal map"); }
              else
              {
                auth_codes_.erase(cit);
              }
            }
            
            certs_.erase(it);
          }
          
          rep->set_type(pb::CertStoreReply::DELETE_KEY);
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
      rep->Clear();
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
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  cert_store_server::~cert_store_server()
  {
  }
    
  void
  cert_store_server::reload_from(const std::string & path)
  {
    std::string inpath{path + "/" + rep_base_type::ep_hash() + "-" + "certstore.data"};
    std::ifstream ifs{inpath};
    if( ifs.good() )
    {
      google::protobuf::io::IstreamInputStream fs(&ifs);
      google::protobuf::io::CodedInputStream stream(&fs);
      
      while( true )
      {
        uint64_t size = 0;
        if( !stream.ReadVarint64(&size) ) break;
        if( size == 0 ) break;
        cert_sptr cptr{new interface::pb::Certificate};
        if( cptr->ParseFromCodedStream(&stream) )
        {
          lock l(mtx_);
          name_key nk{cptr->componentname(), cptr->publickey()};
          certs_[nk] = cptr;
          if( cptr->has_authcode() && !cptr->approved() )
          {
            auth_codes_[cptr->authcode()] = nk;
          }
        }
      }
    }
  }
  
  void
  cert_store_server::save_to(const std::string & path)
  {
    std::string outpath{path + "/" + rep_base_type::ep_hash() + "-" + "certstore.data"};
    std::ofstream of{outpath};
    if( of.good() )
    {
      google::protobuf::io::OstreamOutputStream fs(&of);
      google::protobuf::io::CodedOutputStream stream(&fs);
      
      lock l(mtx_);
      for( auto const & crt : certs_ )
      {
        int bs = crt.second->ByteSize();
        if( bs <= 0 ) continue;
          
        stream.WriteVarint64((uint64_t)bs);
        crt.second->SerializeToCodedStream(&stream);
      }
    }    
  }  
  
}}
