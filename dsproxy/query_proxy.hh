#pragma once

#include <connector/query_server.hh>
#include <connector/query_client.hh>
#include <data.pb.h>
#include <mutex>
#include <memory>
#include <map>
#include <set>
#include <functional>

namespace virtdb { namespace dsproxy {
  
  class query_proxy final
  {
  public:
    enum action {
      forward_query = 1,
      dont_forward  = 2,
    };

    typedef connector::query_server::query_sptr         query_sptr;
    typedef std::set<std::string>                       string_set;
    typedef std::function<action(const std::string &,
                                 query_sptr q)>         on_new_query;
    typedef std::function<void(void)>                   on_disconnect;
    typedef std::function<bool(const std::string &,     // query_id
                               std::set<std::string> &, // columns
                               std::set<uint64_t> &)>   on_resend_chunk;
    
  private:
    typedef std::shared_ptr<connector::query_client>    client_sptr;
    
    connector::query_server        server_;
    client_sptr                    client_sptr_;
    connector::endpoint_client *   ep_client_;
    on_new_query                   on_new_query_;
    on_disconnect                  on_disconnect_;
    on_resend_chunk                on_resend_chunk_;
    mutable std::mutex             mtx_;
    
    void reset_client();
    void handle_query(connector::query_server::query_sptr q);
    
  public:
    bool reconnect(const std::string & server);
    bool reconnect();
    void watch_new_queries(on_new_query);
    void watch_disconnect(on_disconnect);
    void watch_resend_chunk(on_resend_chunk);
    void remove_watch();
    query_proxy(connector::config_client & cfg_client);
    ~query_proxy();
    
  private:
    query_proxy() = delete;
    query_proxy(const query_proxy &) = delete;
    query_proxy& operator=(const query_proxy &) = delete;
  };

}}
