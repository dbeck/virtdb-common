#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include <connector/meta_data_server.hh>
#include <regex>

using namespace virtdb::interface;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  meta_data_server::meta_data_server(server_context::sptr ctx,
                                     config_client & cfg_client)
  : rep_base_type(ctx,
                  cfg_client,
                  std::bind(&meta_data_server::process_replies,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  std::bind(&meta_data_server::publish_meta,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  pb::ServiceType::META_DATA),
    pub_base_type(ctx,
                  cfg_client,
                  pb::ServiceType::META_DATA),
    ctx_{ctx}
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      ep_data.set_svctype(pb::ServiceType::META_DATA);
      ep_data.set_cmd(pb::EndpointData::ADD);
      ep_data.set_validforms(DEFAULT_ENDPOINT_EXPIRY_MS);
      ep_data.add_connections()->MergeFrom(rep_base_type::conn());
      ep_data.add_connections()->MergeFrom(pub_base_type::conn());      
      cfg_client.get_endpoint_client().register_endpoint(ep_data);
      ctx->add_endpoint(ep_data);
    }
  }
  
  void
  meta_data_server::publish_meta(const rep_base_type::req_item&,
                                 rep_base_type::rep_item_sptr)
  {
    // TODO : what do we publish here ... ????
  }
  
  meta_data_server::rep_base_type::rep_item_sptr
  meta_data_server::get_wildcard_data()
  {
    rep_base_type::rep_item_sptr ret;
    {
      lock wcl(wildcard_cache_mtx_);
      ret = wildcard_cache_;
    }
    return ret;
  }
  
  void
  meta_data_server::update_wildcard_data()
  {
    rep_base_type::rep_item_sptr rep{new rep_item};
    
    {
      ctx_->increase_stat("Update cached wildcard metadata");
      lock l(tables_mtx_);
      for( const auto & it : tables_ )
      {
        auto tmp_tab = rep->add_tables();
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
    
    {
      lock wcl(wildcard_cache_mtx_);
      wildcard_cache_ = rep;
    }
  }
  
  void
  meta_data_server::process_replies(const rep_base_type::req_item & req,
                                    rep_base_type::send_rep_handler handler)
  {
    {
      lock l(watch_mtx_);
      if( on_request_ )
      {
        try
        {
          on_request_(req);
        }
        catch(const std::exception & e)
        {
          std::string text{e.what()};
          LOG_ERROR("exception" << V_(text));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception in on_request function");
        }
      }
    }
    
    rep_base_type::rep_item_sptr rep;

    if( req.has_schema() &&
        req.has_name() &&
        req.schema() == ".*" &&
        req.name() == ".*" )
    {
      
      rep = get_wildcard_data();
      LOG_INFO("returning cached wildcard metadata");
    }
    
    if( rep )
    {
      ctx_->increase_stat("Returing cached wildcard metadata");
    }
    else
    {
      rep.reset(new rep_item);
      
      {
        lock l(tables_mtx_);
        bool match_schema = (req.has_schema() && !req.schema().empty());
        
        std::regex table_regex{req.name(), std::regex::extended};
        std::regex schema_regex;
        
        if( match_schema )
          schema_regex.assign(req.schema(), std::regex::extended);
        
        bool with_fields = req.withfields();
        
        for( const auto & it : tables_ )
        {
          
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
        
        {
          LOG_TRACE("processing" << M_(req) << M_(*rep));
        }
      }
    }
    
    if( rep->tables_size() == 0 )
    {
      ctx_->increase_stat("Error gathering metadata");
      LOG_ERROR("couldn't gather metadata for" << M_(req));
    }
    
    handler(rep, false);
  }
  
  void
  meta_data_server::watch_requests(on_request mon)
  {
    lock l(watch_mtx_);
    on_request_ = mon;
  }
  
  void
  meta_data_server::remove_watch()
  {
    lock l(watch_mtx_);
    on_request empty;
    on_request_.swap(empty);
  }
  
  void
  meta_data_server::add_table(table_sptr table)
  {
    // TODO: replace map<> with a better structure, more optimal for regex search
    lock l(tables_mtx_);
    table_map::key_type key(table->schema(), table->name());
    auto it = tables_.find(key);
    if( it == tables_.end() )
    {
      ctx_->increase_stat("Table metadata added");
      tables_.insert(std::make_pair(key,table));
    }
    else
    {
      ctx_->increase_stat("Table metadata updated");
      it->second = table;
    }
  }
  
  void
  meta_data_server::remove_table(const std::string & schema,
                                 const std::string & name)
  {
    lock l(tables_mtx_);
    table_map::key_type key(schema, name);
    tables_.erase(key);
    ctx_->increase_stat("Table metadata removed");
  }
  
  bool
  meta_data_server::has_table(const std::string & schema,
                              const std::string & name)
  {
    lock l(tables_mtx_);
    table_map::key_type key(schema, name);
    return (tables_.count(key) > 0);
  }
  
  meta_data_server::table_sptr
  meta_data_server::get_table(const std::string & schema,
                              const std::string & name)
  {
    lock l(tables_mtx_);
    table_map::key_type key(schema, name);
    auto it = tables_.find(key);
    if( it == tables_.end() )
      return table_sptr();
    else
      return it->second;
  }
  
  bool
  meta_data_server::has_fields(const std::string & schema,
                               const std::string & name)
  {
    lock l(tables_mtx_);
    table_map::key_type key(schema, name);
    auto it = tables_.find(key);
    if( it == tables_.end() )
      return false;
    else if( it->second->fields_size() > 0 )
      return true;
    else
      return false;
  }

  meta_data_server::~meta_data_server() {}
}}

