#pragma once

#include "rep_server.hh"
#include "pub_server.hh"
#include <meta_data.pb.h>
#include <map>
#include <mutex>

namespace virtdb { namespace connector {
  
  class meta_data_server final :
      public rep_server<interface::pb::MetaDataRequest,
                        interface::pb::MetaData>,
      public pub_server<interface::pb::MetaData>
  {
  public:
    typedef std::shared_ptr<interface::pb::TableMeta>          table_sptr;
    
  private:
    typedef rep_server<interface::pb::MetaDataRequest,
                       interface::pb::MetaData>                rep_base_type;
    typedef pub_server<interface::pb::MetaData>                pub_base_type;
    typedef std::pair<std::string,std::string>                 schema_table;
    typedef std::map<schema_table, table_sptr>                 table_map;
    typedef std::lock_guard<std::mutex>                        lock;
    
    // TODO: replace map<> with a better structure, more optimal for regex search
    
    table_map   tables_;
    std::mutex  mtx_;
    
    void publish_meta(rep_base_type::rep_item_sptr);
    void process_replies(const rep_base_type::req_item & req,
                         rep_base_type::send_rep_handler handler);
    
  public:
    meta_data_server(config_client & cfg_client);
    virtual ~meta_data_server();
    void add_table(table_sptr table);
  };
  
}}
