#pragma once

#include <connector/column_server.hh>
#include <connector/column_client.hh>
#include <connector/endpoint_client.hh>
#include <meta_data.pb.h>
#include <mutex>
#include <memory>
#include <functional>
#include <set>
#include <deque>
#include <map>
#include <tuple>

namespace virtdb { namespace dsproxy {
  
  class column_proxy final
  {
    
  public:
    typedef std::function<void(void)> on_disconnect;
    typedef std::vector<std::string> other_channels;
    
    typedef std::function<void(const std::string & provider_name,
                               const std::string & channel,
                               const std::string & subscription,
                               std::shared_ptr<interface::pb::Column> data)> on_data;
  private:
    typedef std::shared_ptr<connector::column_client>        client_sptr;
    typedef std::shared_ptr<interface::pb::Column>           data_sptr;
    
    connector::column_server       server_;
    client_sptr                    client_sptr_;
    connector::endpoint_client *   ep_client_;
    on_data                        handler_;
    on_disconnect                  on_disconnect_;
    std::set<std::string>          subscriptions_;
    std::mutex                     mtx_;
    
    void reset_client();
    void handle_data(const std::string & provider_name,
                     const std::string & channel,
                     const std::string & subscription,
                     std::shared_ptr<interface::pb::Column> data);
  public:
    void subscribe_query(const std::string & query_id);
    void unsubscribe_query(const std::string & query_id);
    bool reconnect(const std::string & server);
    bool reconnect();
    void watch_disconnect(on_disconnect);
    
    column_proxy(connector::config_client & cfg_client,
                 on_data handler);
    ~column_proxy();
    
    void publish(std::shared_ptr<interface::pb::Column> data);
    
  private:
    column_proxy() = delete;
    column_proxy(const column_proxy &) = delete;
    column_proxy& operator=(const column_proxy &) = delete;
  };
}}
