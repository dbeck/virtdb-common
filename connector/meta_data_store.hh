#pragma once

#include <meta_data.pb.h>
#include <map>
#include <mutex>
#include <memory>

namespace virtdb { namespace connector {
  
  class meta_data_store
  {
  public:
    typedef std::shared_ptr<interface::pb::TableMeta>     table_sptr;
    typedef std::shared_ptr<interface::pb::MetaData>      meta_sptr;
    typedef std::pair<std::string,std::string>            schema_table;
    
  private:
    typedef std::map<schema_table, table_sptr>            table_map;
    typedef std::lock_guard<std::mutex>                   lock;
    
    table_map             tables_;
    meta_sptr             wildcard_cache_;
    mutable std::mutex    tables_mtx_;
    mutable std::mutex    wildcard_mtx_;
    
  public:
    typedef std::shared_ptr<meta_data_store> sptr;
    
    meta_data_store();
    virtual ~meta_data_store();
    
    void add_table(table_sptr table);
    
    void remove_table(const std::string & schema,
                      const std::string & name);
    
    table_sptr get_table(const std::string & schema,
                         const std::string & name);
    
    meta_sptr get_tables_regexp(const std::string & schema_regexp,
                                const std::string & table_regexp,
                                bool with_fields);
    meta_sptr get_wildcard_data();
    
    void update_wildcard_data();
    
    bool has_table(const std::string & schema,
                   const std::string & name) const;
    
    bool has_fields(const std::string & schema,
                    const std::string & name) const;
    
    size_t size() const;
  };
  
}}
