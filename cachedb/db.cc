#include "db.hh"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/slice.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/filter_policy.h>
#include <map>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  struct db::impl
  {
    typedef std::shared_ptr<rocksdb::DB>                      db_sptr;
    typedef std::shared_ptr<rocksdb::ColumnFamilyHandle>      cf_handle_sptr;
    typedef std::map<std::string, size_t>                     prefix_map;
    typedef std::map<std::string, cf_handle_sptr>             cf_handle_map;
    
    struct column_family
    {
      typedef std::shared_ptr<column_family>      sptr;
      size_t                                      prefix_len_;
      std::string                                 name_;
      cf_handle_sptr                              handle_sptr_;

      template <typename T>
      static void set_options(T & t, size_t prefix_len)
      {
        t.prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(prefix_len));
        t.memtable_prefix_bloom_bits = 100000000;
        t.memtable_prefix_bloom_probes = 6;
        t.compression = rocksdb::kLZ4Compression;

        rocksdb::BlockBasedTableOptions topts;
        // Enable prefix bloom for SST files
        topts.index_type = rocksdb::BlockBasedTableOptions::kHashSearch;
        topts.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10,true));

        // pass table options to t
        t.table_factory.reset(rocksdb::NewBlockBasedTableFactory(topts));
      }
      
      column_family(size_t prefix_len,
                    cf_handle_sptr h)
      : prefix_len_{prefix_len_},
        name_{h->GetName()},
        handle_sptr_{h}
      {
      }
  
      ~column_family()
      {
        handle_sptr_.reset();
      }
    };
    
    typedef std::map<std::string, column_family::sptr>  column_family_map;
    
    std::string                      path_;
    column_family_map                column_families_;
    db_sptr                          db_;
    rocksdb::Options                 options_;
    
    db_sptr
    open_db(const std::string & path,
            const prefix_map & desired,
            prefix_map & actual,
            prefix_map & missing,
            cf_handle_map & cf_handles)
    {
      using namespace rocksdb;
      db_sptr ret;
      
      std::vector<std::string> tmp_cf;
      Status s = DB::ListColumnFamilies(options_,
                                        path,
                                        &tmp_cf);
      if( !s.ok() )
      {
        LOG_TRACE("cannot query list of column families" << V_(path));
      }
      
      // clear inputs to make sure we are not colliding with previous runs
      actual.clear();
      missing.clear();
      
      std::set<std::string> cf_set{tmp_cf.begin(), tmp_cf.end()};
      for( auto const & c : desired )
      {
        if( cf_set.count(c.first) > 0 )
          actual.insert(c);
        else
          missing.insert(c);
      }
      
      // open DB with the current column families
      std::vector<ColumnFamilyDescriptor> cfds;
      
      ColumnFamilyOptions cfo_default;
      column_family::set_options(cfo_default, 16);
      
      std::vector<size_t> prefix_lens;
      
      // have to open default column family
      cfds.push_back(ColumnFamilyDescriptor(kDefaultColumnFamilyName, cfo_default));
      prefix_lens.push_back(16);
      
      for( auto const & cf : actual )
      {
        ColumnFamilyOptions opt;
        column_family::set_options(opt, cf.second);
        // add the existing ones too
        cfds.push_back(ColumnFamilyDescriptor(cf.first, opt));
        prefix_lens.push_back(cf.second);
      }
      
      // now we can open the database
      std::vector<ColumnFamilyHandle*> handles;
      
      DB * db = nullptr;
      s = DB::Open(options_, path, cfds, &handles, &db);
      
      if( !s.ok() )
      {
        LOG_ERROR("failed to open database" <<
                  V_(path) <<
                  V_(desired.size()) <<
                  V_(actual.size()) <<
                  V_(missing.size()));
        
        return ret;
      }
      
      // save DB handle
      ret.reset(db);
      
      // save column family handles
      for( int i=0; i<cfds.size(); ++i )
      {
        cf_handles[handles[i]->GetName()] = cf_handle_sptr(handles[i]);
      }
      return ret;
    }

    bool
    create_column_families(db_sptr dbsp,
                           const prefix_map & desired,
                           cf_handle_map & cf_handles)
    {
      using namespace rocksdb;
      bool ret = true;
      
      for( auto const & cf : desired )
      {
        ColumnFamilyOptions opt;
        column_family::set_options(opt, cf.second);
        ColumnFamilyDescriptor desc(cf.first, opt);
        ColumnFamilyHandle * handle = 0;
        Status s = dbsp->CreateColumnFamily(opt,
                                            cf.first,
                                            &handle);
        if( s.ok() && handle )
        {
          cf_handles[handle->GetName()] = cf_handle_sptr(handle);
        }
        else
        {
          LOG_ERROR("failed to create column family" <<  V_(cf.first) << V_(cf.second));
          ret = false;
        }
      }
      
      return ret;
    }

    bool
    init(const std::string & path,
         const storeable_ptr_vec_t & stvec)
    {
      if( stvec.empty() )
      {
        LOG_ERROR("empty storable vector passed as an argument" << V_(path));
        return false;
      }
      
      // drop old database objects
      cleanup();
      
      db_sptr db;
      prefix_map desired;
      prefix_map actual;
      prefix_map missing;
      cf_handle_map cf_handles;
      
      // gather desired familues
      for( auto const & s : stvec )
      {
        if( s )
        {
          auto const & cs = s->column_set();
          for( auto const & cf : cs )
          {
            desired.insert(std::make_pair(cf,s->key_len()));
          }
        }
      }
      
      db = open_db(path, desired, actual, missing, cf_handles);
      
      if( !db )
      {
        LOG_ERROR("failed to open database" << V_(path) << V_(desired.size()) );
        return false;
      }
      
      if( missing.size() > 0 )
      {
        cf_handle_map new_handles;
        if( !create_column_families(db,
                                    missing,
                                    new_handles) )
        {
          LOG_ERROR("couldn't create column families" << V_(path) << V_(missing.size()));
          return false;
        }
        
        // cleanup database and try to reopen
        for( auto & h : new_handles ) h.second.reset();
        for( auto & h : cf_handles )  h.second.reset();
        db.reset();
        actual.clear();
        missing.clear();
        
        db = open_db(path, desired, actual, missing, cf_handles);
      }

      // save column handles
      for( auto & cfh : cf_handles )
      {
        size_t prefix_len = desired[cfh.first];
        column_family::sptr cfsp{new column_family{prefix_len, cfh.second}};
        column_families_[cfh.first] = cfsp;
      }
      
      db_ = db;
      
      // now we have valid database objects
      return true;
    }
    
    impl() : db_{nullptr}
    {
      // set options for default coumn family
      column_family::set_options(options_, 16);
      
      // DB specific options
      options_.create_if_missing   = true;
      options_.keep_log_file_num   = 10;
      options_.max_log_file_size   = 10 * 1024 * 1024;
      options_.max_open_files      = -1;
    }
    
    void
    cleanup()
    {
      column_families_.clear();
      db_.reset();
    }
    
    std::set<std::string>
    column_families() const
    {
      std::set<std::string> ret;
      for( auto const & cf : column_families_ )
        ret.insert(cf.first);
      return ret;
    }
    
    ~impl()
    {
      cleanup();
    }
  };
  
  // forwarders:
  bool
  db::init(const std::string & path,
           const storeable_ptr_vec_t & stvec)
  {
    return impl_->init(path, stvec);
  }
  
  std::set<std::string>
  db::column_families() const
  {
    return impl_->column_families();
  }
  
  db::db() : impl_{new impl} {}
  db::~db() {}
}}
