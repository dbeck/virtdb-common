#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <logger.hh>
#include <util/exception.hh>
#include <connector/meta_data_store.hh>
#include <regex>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  meta_data_store::meta_data_store(const std::string & name)
  : name_{name}
  {
  }
  
  meta_data_store::~meta_data_store() {}
  
  void
  meta_data_store::add_table(table_sptr table)
  {
    table_map::key_type key;
    if( table->has_schema() )
    {
      key.first = table->schema();
    }
    
    if( !table->has_name() )
    {
      THROW_("table name is empty");
    }
    key.second = table->name();
    
    {
      lock lck(tables_mtx_);
      // LOG_TRACE("updating" << V_(tables_.count(key)) << V_(key.first) << V_(key.second) << V_(table->fields_size()));
      tables_[key] = table;
    }
  }
   
  void
  meta_data_store::remove_table(const std::string & schema,
                                const std::string & name)
  {
    table_map::key_type key{schema,name};
    {
      lock lck(tables_mtx_);
      auto it = tables_.find(key);
      if( it != tables_.end() )
        tables_.erase(it);
    }
  }
   
  meta_data_store::table_sptr
  meta_data_store::get_table(const std::string & schema,
                             const std::string & name)
  {
    table_map::key_type key{schema,name};
    table_sptr ret;
    {
      lock lck(tables_mtx_);
      auto it = tables_.find(key);
      if( it != tables_.end() )
        ret = it->second;
    }
    return ret;
  }
  
  bool
  meta_data_store::has_table(const std::string & schema,
                             const std::string & name) const
  {
    table_map::key_type key{schema,name};
    lock lck(tables_mtx_);
    return (tables_.count(key) > 0);
  }
   
  bool
  meta_data_store::has_fields(const std::string & schema,
                              const std::string & name) const
  {
    table_map::key_type key{schema,name};
    table_sptr ret;
    {
      lock lck(tables_mtx_);
      auto it = tables_.find(key);
      if( it != tables_.end() )
      {
        if( it->second->fields_size() > 0 )
          return true;
      }
    }
    return false;
  }
  
  size_t
  meta_data_store::size() const
  {
    size_t ret = 0;
    {
      lock lck(tables_mtx_);
      ret = tables_.size();
    }
    LOG_TRACE(V_(name_) << V_(ret));
    return ret;
  }
   
  meta_data_store::meta_sptr
  meta_data_store::get_wildcard_data()
  {
    meta_sptr ret;
    {
      lock lck(wildcard_mtx_);
      ret = wildcard_cache_;
    }
    return ret;
  }

  void
  meta_data_store::update_wildcard_data()
  {
    meta_sptr res{new interface::pb::MetaData};
    // TODO: we should refresh this from time to time
    {
      size_t added = 0;
      lock l(tables_mtx_);
      for( const auto & it : tables_ )
      {
        auto tmp_tab = res->add_tables();
        // selectively merge everything except fields
        if( it.second->has_name() )
          tmp_tab->set_name(it.second->name());
        
        if( it.second->has_schema() )
          tmp_tab->set_schema(it.second->schema());
        
        for( auto const & c : it.second->comments() )
          tmp_tab->add_comments()->MergeFrom(c);
        
        for( auto const & p : it.second->properties() )
          tmp_tab->add_properties()->MergeFrom(p);
        
        ++added;
      }
      LOG_TRACE(V_(name_) << V_(added) << "tables to result");
    }
    
    {
      lock wcl(wildcard_mtx_);
      if( res->tables_size() > 0 )
      {
        // never cache empty wildcard data
        wildcard_cache_ = res;
      }
      else
      {
        LOG_TRACE(V_(name_) << "not updating wildcard cache with empty metadata");
      }
    }
  }
  
  meta_data_store::meta_sptr
  meta_data_store::get_tables_regexp(const std::string & schema_regexp,
                                     const std::string & table_regexp,
                                     bool with_fields)
  {
    LOG_TRACE(V_(name_) << V_(schema_regexp) << V_(table_regexp) << V_(with_fields));

    meta_data_store::meta_sptr rep;
    if( table_regexp.empty() ) return rep;
    
    lock l(tables_mtx_);
    bool match_schema = !schema_regexp.empty();

    LOG_TRACE(V_(name_) << V_(match_schema) << V_(table_regexp) << V_(tables_.size()));
    
    std::regex table_regex{table_regexp, std::regex::extended};
    std::regex schema_regex;
    
    if( match_schema )
      schema_regex.assign(schema_regexp, std::regex::extended);
    
    rep.reset(new interface::pb::MetaData);
    
    for( const auto & it : tables_ )
    {
      // LOG_TRACE("matching" << V_(it.second->name()) << V_(table_regexp));
      if( std::regex_match(it.second->name(),
                           table_regex,
                           std::regex_constants::match_any |
                           std::regex_constants::format_sed ) )
      {
        
        if( !match_schema || std::regex_match(it.second->schema(),
                                              schema_regex,
                                              std::regex_constants::match_any |
                                              std::regex_constants::format_sed ) )
        {
          auto tmp_tab = rep->add_tables();
          if( with_fields )
          {
            if( !it.second )
            {
              LOG_ERROR("missing field info for" << V_(table_regexp) << V_(with_fields));
              return meta_data_store::meta_sptr();
            }
            // merge everything
            tmp_tab->MergeFrom(*(it.second));
          }
          else
          {
            // selectively merge everything except fields
            if( it.second->has_name() )
              tmp_tab->set_name(it.second->name());
            
            if( it.second->has_schema() )
              tmp_tab->set_schema(it.second->schema());
            
            for( auto const & c : it.second->comments() )
              tmp_tab->add_comments()->MergeFrom(c);
            
            for( auto const & p : it.second->properties() )
              tmp_tab->add_properties()->MergeFrom(p);
          }
        }
      }
    }
    
    return rep;
  }
  
  const std::string &
  meta_data_store::name() const
  {
    return name_;
  }

  
}}
