#pragma once

#include <connector/column_server.hh>
#include <connector/column_client.hh>
#include <connector/endpoint_client.hh>
#include <util/timer_service.hh>
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
                               std::shared_ptr<interface::pb::Column> data,
                               std::string & new_channel_id,
                               other_channels & others)> on_data;
  private:
    typedef std::shared_ptr<connector::column_client>        client_sptr;
    typedef std::shared_ptr<interface::pb::Column>           data_sptr;
    typedef std::deque<data_sptr>                            data_backlog;
    typedef std::map<std::string, data_backlog>              backlog_map;
    typedef std::tuple<std::string, uint64_t>                message_id;
    typedef std::map<message_id, data_sptr>                  message_cache;
    typedef std::set<uint64_t>                               id_set;
    typedef std::map<std::string, id_set>                    blockid_map;
    
    connector::column_server       server_;
    client_sptr                    client_sptr_;
    connector::endpoint_client *   ep_client_;
    on_data                        handler_;
    on_disconnect                  on_disconnect_;
    std::set<std::string>          subscriptions_;
    std::mutex                     mtx_;
    backlog_map                    backlog_;
    std::mutex                     backlog_mtx_;
    util::timer_service            timer_svc_;
    message_cache                  message_cache_;
    std::mutex                     message_cache_mtx_;
    blockid_map                    block_ids_;
    std::mutex                     block_id_mtx_;
    
    void reset_client();
    void handle_data(const std::string & provider_name,
                     const std::string & channel,
                     const std::string & subscription,
                     std::shared_ptr<interface::pb::Column> data);
    
    void add_to_backlog(const std::string & query_id, data_sptr);
    void add_to_message_cache(const std::string & channel_id,
                              const std::string & col_name,
                              uint64_t block_id,
                              data_sptr);
    
  public:
    bool resend_message(const std::string & query_id,
                        const std::string & segment_id,
                        const std::string & col_name,
                        uint64_t block_id);
    void send_backlog(const std::string & query_id,
                      const std::string & segment_id);
    void subscribe_query(const std::string & query_id);
    void unsubscribe_query(const std::string & query_id);
    bool reconnect(const std::string & server);
    bool reconnect();
    void watch_disconnect(on_disconnect);
    column_proxy(connector::config_client & cfg_client,
                 on_data handler);
    ~column_proxy();
    
  private:
    column_proxy() = delete;
    column_proxy(const column_proxy &) = delete;
    column_proxy& operator=(const column_proxy &) = delete;
  };
}}
