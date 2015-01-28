#pragma once

#include <connector/meta_data_server.hh>
#include <connector/meta_data_client.hh>
#include <connector/endpoint_client.hh>
#include <util/timer_service.hh>
#include <meta_data.pb.h>
#include <mutex>
#include <memory>

namespace virtdb { namespace dsproxy {
  
  class meta_proxy final
  {
  public:
    typedef std::function<void(void)> on_disconnect;
  private:
    typedef std::shared_ptr<connector::meta_data_client>  client_sptr;
    
    connector::meta_data_server   server_;
    client_sptr                   client_sptr_;
    connector::endpoint_client *  ep_client_;
    util::timer_service           timer_service_;
    on_disconnect                 on_disconnect_;
    std::mutex                    mtx_;
    
    void reset_client();

  public:
    void watch_disconnect(on_disconnect);
    bool reconnect();
    bool reconnect(const std::string & server);
    meta_proxy(connector::config_client & cfg_client);
    ~meta_proxy();
    
  private:
    meta_proxy() = delete;
    meta_proxy(const meta_proxy &) = delete;
    meta_proxy & operator=(const meta_proxy&) = delete;
  };
}}
