#pragma once

#include "rep_server.hh"
#include "pub_server.hh"
#include <svc_config.pb.h>
#include <util/async_worker.hh>
#include <util/zmq_utils.hh>
#include "config_client.hh"
#include "endpoint_server.hh"
#include <map>

namespace virtdb { namespace connector {
  
  class config_server :
      public rep_server<interface::pb::Config,
                        interface::pb::Config>,
      public pub_server<interface::pb::Config>
  {
  public:
    typedef rep_server<interface::pb::Config,
                       interface::pb::Config>     rep_base_type;
    typedef pub_server<interface::pb::Config>     pub_base_type;
    
  private:
    typedef std::map<std::string, interface::pb::Config>    config_map;
    typedef std::map<std::string, std::string>              cfg_hash_map;
    typedef std::lock_guard<std::mutex>                     lock;
    
    config_map               configs_;
    cfg_hash_map             hashes_;
    mutable std::mutex       mtx_;

    void
    on_reply_fwd(const rep_base_type::req_item &,
                rep_base_type::rep_item_sptr);
    
    void
    on_request_fwd(const rep_base_type::req_item &,
                   rep_base_type::send_rep_handler);
    
  protected:
    virtual void
    on_reply(const rep_base_type::req_item &,
             rep_base_type::rep_item_sptr);
    
    virtual void
    on_request(const rep_base_type::req_item &,
               rep_base_type::send_rep_handler);
    
  public:
    config_server(server_context::sptr ctx,
                  config_client & cfg_client);
    virtual ~config_server();
    
    virtual void reload_from(const std::string & path);
    virtual void save_to(const std::string & path);
    
    static util::zmq_socket_wrapper::host_set
    endpoint_hosts(const endpoint_server & ep_server);
  };
}}
