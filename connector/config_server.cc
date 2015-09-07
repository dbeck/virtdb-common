#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "config_server.hh"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <logger.hh>
#include <util/net.hh>
#include <util/flex_alloc.hh>
#include <util/hex_util.hh>
#include <xxhash.h>
#include <fstream>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  config_server::config_server(server_context::sptr ctx,
                               config_client & cfg_client)
  :
    rep_base_type(ctx,
                  cfg_client,
                  std::bind(&config_server::on_request_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&config_server::on_reply_fwd,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::CONFIG),
    pub_base_type(ctx,
                  cfg_client,
                  pb::ServiceType::CONFIG)
  {
    // setting up our own endpoints
    pb::EndpointData ep_data;
    {
      ep_data.set_name(ctx->service_name());
      ep_data.set_svctype(pb::ServiceType::CONFIG);
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }

  void
  config_server::on_reply_fwd(const rep_base_type::req_item & request,
                              rep_base_type::rep_item_sptr rep)
  {
    on_reply(request, rep);
  }
  
  void
  config_server::on_reply(const rep_base_type::req_item & request,
                          rep_base_type::rep_item_sptr rep)
  {
    if( rep && rep->has_name() )
    {
      // generate a hash on the request
      std::string hash64, hash32;
      int bs = request.ByteSize();
      if( bs > 0 )
      {
        util::flex_alloc<uint8_t, 512> buffer(bs);
        if( request.SerializeToArray(buffer.get(), bs) )
        {
          util::hex_util(XXH64(buffer.get(), bs, 0xDeadBeaf), hash64);
          util::hex_util(XXH32(buffer.get(), bs, 0), hash32);
        }
      }
      
      if( hash32.empty() ) hash32 = std::to_string(std::rand());
      if( hash64.empty() ) hash64 = std::to_string(std::rand());
      
      std::string hash{hash32+hash64};
      std::string subscription{rep->name()};
      
      bool suppress = false;
      {
        lock l(mtx_);
        auto it = hashes_.find(request.name());
        if( it != hashes_.end() && it->second == hash && !hash.empty() )
          suppress = true;
        else
          hashes_[request.name()] = hash;
      }
      
      LOG_TRACE("publishing config" << V_(subscription) << V_(rep->name()) << V_(hash) << V_(suppress));
      if( !suppress )
      {
        publish(subscription,rep);
      }
    }
  }

  void
  config_server::on_request_fwd(const rep_base_type::req_item & request,
                                rep_base_type::send_rep_handler handler)
  {
    on_request(request, handler);
  }
  
  void
  config_server::on_request(const rep_base_type::req_item & request,
                            rep_base_type::send_rep_handler handler)
  {
    rep_base_type::rep_item_sptr ret;
    LOG_TRACE("config request arrived" << M_(request));
    
    {
      lock l(mtx_);
      auto cfg_it = configs_.find(request.name());
      
      if( cfg_it != configs_.end() && !request.configdata_size() )
      {
        ret = rep_item_sptr{new rep_base_type::rep_item(cfg_it->second)};
      }
      
      if( request.configdata_size() )
      {
        // save config data
        if( cfg_it != configs_.end() )
          configs_.erase(cfg_it);
        
        configs_[request.name()] = request;
      }
      
      if( !ret )
      {
        // send back the original request
        ret = rep_item_sptr{new rep_base_type::rep_item(request)};
      }
    }
    
    handler(ret,false);
  }
  
  void
  config_server::reload_from(const std::string & path)
  {
    std::string inpath{path + "/" + pub_base_type::ep_hash() + "-" + "config.data"};
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
        pb::Config cfg;
        if( cfg.ParsePartialFromCodedStream(&stream) )
        {
          //
          lock l(mtx_);
          configs_[cfg.name()] = cfg;
        }
      }
    }
  }
  
  void
  config_server::save_to(const std::string & path)
  {
    std::string outpath{path + "/" + pub_base_type::ep_hash() + "-" + "config.data"};
    std::ofstream of{outpath};
    if( of.good() )
    {
      google::protobuf::io::OstreamOutputStream fs(&of);
      google::protobuf::io::CodedOutputStream stream(&fs);
      
      lock l(mtx_);
      for( auto const & cfg : configs_ )
      {
        pb::Config cfgsvd{cfg.second};
        if( cfgsvd.has_name() == false )
          cfgsvd.set_name(cfg.first);
        
        int bs = cfgsvd.ByteSize();
        if( bs <= 0 ) continue;
        
        stream.WriteVarint64((uint64_t)bs);
        cfgsvd.SerializePartialToCodedStream(&stream);
      }
    }
  }
  
  config_server::~config_server() {}
}}
