#include "query_server.hh"
#include <functional>

using namespace virtdb::interface;

namespace virtdb { namespace connector {
  
  query_server::query_server(config_client & cfg_client)
  : base_type(cfg_client,
              std::bind(&query_server::handler_function,
                        this,
                        std::placeholders::_1))
  {
    pb::EndpointData ep_data;
    {
      ep_data.set_name(cfg_client.get_endpoint_client().name());
      auto conn = ep_data.add_connections();
      conn->MergeFrom(base_type::conn());
    }
    cfg_client.get_endpoint_client().register_endpoint(ep_data);
  }
  
  namespace
  {
    template <typename FUN, typename ITEM>
    void call_monitor(FUN & fun,
                      ITEM & item,
                      const std::string & src)
    {
      try
      {
        fun(src, item);
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown excpetion caught");
      }
    }
    
    template <typename CONTAINER, typename ITEM>
    void fire_monitors(CONTAINER & container,
                       ITEM & item,
                       const std::string & key,
                       const std::string & src)
    {
      try
      {
        auto it = container.find(key);
        if( it != container.end() )
          call_monitor(it->second, item, src);
      }
      catch (const std::exception & e)
      {
        std::string text{e.what()};
        LOG_ERROR("exception caught" << V_(text));
      }
      catch( ... )
      {
        LOG_ERROR("unknown excpetion caught");
      }
    }
    
    std::string gen_table_key(const std::string & query_id,
                              const std::string & schema,
                              const std::string & table)
    {
      std::ostringstream os;
      os << query_id << '/' << schema << '/' << table;
      return os.str();
    }
    
  }
  
  void
  query_server::handler_function(query_sptr qsptr)
  {
    {
      lock l(monitors_mtx_);
      // query monitors
      const std::string & n = name();
      fire_monitors(query_monitors_, qsptr, qsptr->queryid(), n);
      fire_monitors(query_monitors_, qsptr, "", n);
      
      std::string query_id = qsptr->queryid();
      std::string schema   = qsptr->schema();
      std::string table    = qsptr->table();
      
      // table monitors
      if( !table_monitors_.empty() )
      {
        fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,schema,table), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,schema,""), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,"",table), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key("",schema,table), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key(query_id,"",""), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key("",schema,""), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key("","",table), n);
        fire_monitors(table_monitors_, qsptr, gen_table_key("","",""), n);
      }
    }
  }
  
  void
  query_server::watch(const std::string & query_id,
                      query_monitor mon)
  {
    lock l(monitors_mtx_);
    query_monitors_[query_id] = mon;
  }

  void
  query_server::remove_watch(const std::string & query_id)
  {
    lock l(monitors_mtx_);
    query_monitors_.erase(query_id);
  }

  void
  query_server::watch(const std::string & query_id,
                      const std::string & schema,
                      const std::string & table,
                      query_monitor mon)
  {
    std::ostringstream os;
    os << query_id << '/' << schema << '/' << table;
    {
      lock l(monitors_mtx_);
      table_monitors_[os.str()] = mon;
    }
  }

  void
  query_server::remove_watch(const std::string & query_id,
                             const std::string & schema,
                             const std::string & table)
  {
    std::ostringstream os;
    os << query_id << '/' << schema << '/' << table;
    {
      lock l(monitors_mtx_);
      table_monitors_.erase(os.str());
    }
  }
  

  void
  query_server::remove_watches()
  {
    lock l(monitors_mtx_);
    query_monitors_.clear();
    table_monitors_.clear();
  }
  
  query_server::~query_server()
  {
  }
  
}}
