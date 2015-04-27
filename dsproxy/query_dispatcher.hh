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
  
  class query_dispatcher final
  {
  public:
    typedef std::set<std::string>                       string_set;
    typedef std::function<void(const std::string &)>    on_new_query;
    typedef std::function<void(const std::string &,
                               const std::string &)>    on_new_segment;
    typedef std::function<void(void)>                   on_disconnect;
    typedef std::function<bool(const std::string &,     // query_id
                               const std::string &,     // segment_id
                               std::set<std::string> &, // columns
                               std::set<uint64_t> &)>   on_resend_chunk;
    
  private:
    typedef std::shared_ptr<connector::query_client>    client_sptr;
    typedef std::map<std::string, string_set>           query_segment_map;
    typedef std::map<std::string, string_set>           stopped_segment_map;
    
    connector::server_context::sptr   server_ctx_;
    connector::client_context::sptr   client_ctx_;
    connector::query_server           server_;
    client_sptr                       client_sptr_;
    connector::endpoint_client *      ep_client_;
    query_segment_map                 query_map_;
    stopped_segment_map               stopped_map_;
    on_new_query                      on_new_query_;
    on_new_segment                    on_new_segment_;
    on_disconnect                     on_disconnect_;
    on_resend_chunk                   on_resend_chunk_;
    mutable std::mutex                mtx_;
    
    void reset_client();
    void handle_query(connector::query_server::query_sptr q);
    
  public:
    string_set query_segments(const std::string & query_id) const;
    size_t query_count() const;
    void remove_query(const std::string & query_id);
    void remove_segments(const std::string & query_id, const string_set & segments);
    bool reconnect(const std::string & server);
    bool reconnect();
    void watch_new_queries(on_new_query);
    void watch_new_segments(on_new_segment);
    void watch_disconnect(on_disconnect);
    void watch_resend_chunk(on_resend_chunk);
    void remove_watch();
    query_dispatcher(connector::server_context::sptr sr_ctx,
                     connector::client_context::sptr cl_ctx,
                     connector::config_client & cfg_client);
    ~query_dispatcher();
    
  private:
    query_dispatcher() = delete;
    query_dispatcher(const query_dispatcher &) = delete;
    query_dispatcher& operator=(const query_dispatcher &) = delete;
  };

}}
