#include "store.hh"
#include <cachedb/dbid.hh>
#include <cachedb/hash_util.hh>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/filter_policy.h>
#include <util/exception.hh>
#include <util/flex_alloc.hh>
#include <logger.hh>
#include <deque>

namespace virtdb { namespace cachedb {
  
  struct store::impl
  {
    std::string                      path_;
    size_t                           expiry_second_;
    size_t                           prefix_len_;
    int                              bits_per_bloom_key_;
    bool                             block_based_bloom_filter_;
    rocksdb::DB *                    db_;
    rocksdb::Options                 options_;
    rocksdb::BlockBasedTableOptions  table_options_;
    
    impl(const std::string & path,
         size_t expiry_sec)
    : path_{path},
      expiry_second_{expiry_sec},
      prefix_len_{expiry_secs_to_prefix_len(expiry_sec)},
      bits_per_bloom_key_{10},
      block_based_bloom_filter_{true},
      db_{nullptr}
    {
      using namespace rocksdb;
      
      // Enable prefix bloom for mem tables
      options_.create_if_missing = true;
      options_.prefix_extractor.reset(NewFixedPrefixTransform(prefix_len_));
      options_.memtable_prefix_bloom_bits = 100000000;
      options_.memtable_prefix_bloom_probes = 6;
      options_.compression = rocksdb::kLZ4Compression;
      options_.keep_log_file_num = 10;
      options_.max_log_file_size = 10 * 1024 * 1024;
      options_.max_open_files = -1;
      
      // Enable prefix bloom for SST files
      table_options_.index_type = rocksdb::BlockBasedTableOptions::kHashSearch;
      table_options_.filter_policy.reset(NewBloomFilterPolicy(bits_per_bloom_key_,
                                                              block_based_bloom_filter_));
      options_.table_factory.reset(NewBlockBasedTableFactory(table_options_));
      
      Status s = DB::Open(options_, path_,  &db_);
      if( !s.ok() ) { THROW_("failed to opend rocksdb database"); }
    }
    
    ~impl()
    {
      if( db_ )
      {
        delete db_;
      }
    }
  };
  
  size_t
  store::expiry_secs_to_prefix_len(size_t expiry_seconds)
  {
    size_t ret = 28;
    if( expiry_seconds > 10       ) --ret;  // YYYYmmddHHM--- 10s
    if( expiry_seconds > 60       ) --ret;  // YYYYmmddHH---- 1m
    if( expiry_seconds > 600      ) --ret;  // YYYYmmddH----- 10m
    if( expiry_seconds > 3600     ) --ret;  // YYYYmmdd------ 1h
    if( expiry_seconds > 36000    ) --ret;  // YYYYmmd------- 10h
    if( expiry_seconds > 86400    ) --ret;  // YYYYmm-------- 1d
    if( expiry_seconds > 864000   ) --ret;  // YYYYm--------- 10d
    if( expiry_seconds > 2592000  ) --ret;  // YYYY---------- 30d
    if( expiry_seconds > 25920000 ) --ret;  // YYY----------- 300d
    return ret;
  }
  
  void
  store::add_column(const interface::pb::Query & query,
                    const time_point query_start_time,
                    const interface::pb::Column & column)
  {
    int field_id = -1;
    int field_count = query.fields_size();
    
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    for( int i=0; i<field_count; ++i )
    {
      auto const & fld = query.fields(i);
      if( fld.name() == column.name() )
      {
        field_id = i;
        break;
      }
    }
    
    if( field_id == -1 )
    {
      LOG_ERROR("column field" << V_(column.name()) << "cannot be found in" << M_(query));
      THROW_("no matching field in query");
    }
      
    dbid id(query,
            field_id,
            query_start_time,
            column.seqno(),
            (column.endofdata()?"e":""));
    
    
    std::string key{id.genkey()};
    
    int byte_size = column.ByteSize();
    if( byte_size <= 0 )
    {
      LOG_ERROR("empty data in column to be written" <<
                M_(query) <<
                V_(key) <<
                V_(field_id) <<
                V_(column.name()) <<
                V_(column.seqno()) <<
                V_(column.endofdata()) <<
                V_(byte_size));
      THROW_("invalid column data received");
    }
    
    util::flex_alloc<char, 1024> buffer(byte_size);
    
    if( !column.SerializeToArray(buffer.get(), byte_size) )
    {
      LOG_ERROR("failed to serialize column" <<
                M_(query) <<
                V_(key) <<
                V_(field_id) <<
                V_(column.name()) <<
                V_(column.seqno()) <<
                V_(column.endofdata()) <<
                V_(byte_size));
      THROW_("failed to serialize column");
    }
    
    rocksdb::Slice slice_val(buffer.get(), byte_size);
    
    auto wropts = rocksdb::WriteOptions();
    wropts.sync = true;

    rocksdb::Status s = impl_->db_->Put(wropts,
                                        key,
                                        slice_val);
    if( !s.ok() )
    {
      LOG_ERROR("failed to store column for query" <<
                M_(query) <<
                V_(key) <<
                V_(field_id) <<
                V_(column.name()) <<
                V_(column.seqno()) <<
                V_(column.endofdata()) <<
                V_(byte_size));
      THROW_("failed to store column");
    }
  }
  
  bool
  store::get_columns(const interface::pb::Query & query,
                     size_t min_block_id,
                     column_allocator allocator,
                     column_handler handler)
  {
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    size_t n_items = 0;
    interface::pb::Column col;
    time_point t = clock::now() - std::chrono::seconds(impl_->expiry_second_);
    dbid id(query, 0, t, 0);
    std::string seek_key{id.genkey()};
    rocksdb::Slice key_slice{seek_key.c_str(), impl_->prefix_len_};
    
    std::string min_block_id_hex;
    hash_util::hex(min_block_id, min_block_id_hex);
    
    std::string key_str;
    std::string block_id;
    
    auto iter = impl_->db_->NewIterator(rocksdb::ReadOptions());
    bool continue_loop = true;
    for( iter->Seek(key_slice);
        (iter->Valid() && continue_loop);
        iter->Next() )
    {
      rocksdb::Slice key{iter->key()};
      rocksdb::Slice value{iter->value()};
      key_str = key.ToString();

      if( ::memcmp(key.data(), key_slice.data(), 16) != 0 )
        break;

      auto item = allocator();
      if( !item ) break;
      dbid::extract_sub_b(key_str, block_id);
      
      if( block_id < min_block_id_hex ) continue;
      
      if( !item->ParseFromArray(value.data(), value.size()) )
      {
        LOG_ERROR("Cannot parse value for" << V_(key_str));
        continue;
      }
      continue_loop = handler(item);
      ++n_items;
      if( !continue_loop )
      {
        LOG_TRACE("Stopping column loop because handler returned false");
      }
    }
    
    LOG_TRACE("Read" << V_(n_items) << "columns");
    return (n_items > 0);
  }
  
  size_t
  store::expire_columns(size_t seconds_range)
  {
    // TODO
    return 0;
  }
  
  void
  store::add_config(const interface::pb::Config & cfg)
  {
    dbid id(cfg);
    std::string key{id.genkey()};
    
    int byte_size = cfg.ByteSize();
    if( byte_size <= 0 )
    {
      LOG_ERROR("empty cfg in column to be written" <<
                M_(cfg) <<
                V_(key) <<
                V_(byte_size));
      THROW_("invalid cfg data");
    }
    
    util::flex_alloc<char, 512> buffer(byte_size);
    
    if( !cfg.SerializeToArray(buffer.get(), byte_size) )
    {
      LOG_ERROR("failed to serialize config" <<
                M_(cfg) <<
                V_(key) <<
                V_(byte_size));
      THROW_("failed to serialize config");
    }
    
    rocksdb::Slice slice_val(buffer.get(), byte_size);
    
    auto wropts = rocksdb::WriteOptions();
    wropts.sync = true;
    
    rocksdb::Status s = impl_->db_->Put(wropts,
                                        key,
                                        slice_val);
    if( !s.ok() )
    {
      LOG_ERROR("failed to store config" <<
                M_(cfg) <<
                V_(key) <<
                V_(byte_size));
      THROW_("failed to store config");
    }
  }
  
  bool
  store::get_configs(config_allocator allocator,
                     config_handler handler)
  {
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    size_t n_items = 0;
    interface::pb::Config cfg;
    cfg.set_name("dummy");
    dbid id(cfg);
    std::string seek_key{id.genkey()};
    rocksdb::Slice key_slice{seek_key.c_str(), impl_->prefix_len_};
    
    std::string key_str;
    
    auto iter = impl_->db_->NewIterator(rocksdb::ReadOptions());
    bool continue_loop = true;
    for( iter->Seek(key_slice);
        (iter->Valid() && continue_loop);
        iter->Next() )
    {
      rocksdb::Slice key{iter->key()};
      rocksdb::Slice value{iter->value()};
      
      if( ::memcmp(key.data(), key_slice.data(), 16) != 0 )
        break;

      auto item = allocator();
      if( !item ) break;
      
      if( !item->ParseFromArray(value.data(), value.size()) )
      {
        key_str = key.ToString();
        LOG_ERROR("Cannot parse value for" << V_(key_str));
        continue;
      }
      continue_loop = handler(item);
      ++n_items;
      if( !continue_loop )
      {
        LOG_TRACE("Stopping cfg loop because handler returned false");
      }
    }
    
    LOG_TRACE("Read" << V_(n_items) << "configs");
    return (n_items > 0);
  }
  
  size_t
  store::remove_configs(config_allocator allocator,
                        config_handler handler)
  {
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    interface::pb::Config cfg;
    cfg.set_name("dummy");
    dbid id(cfg);
    std::string seek_key{id.genkey()};
    rocksdb::Slice key_slice{seek_key.c_str(), impl_->prefix_len_};
    
    std::deque<std::string> to_remove;
    
    auto iter = impl_->db_->NewIterator(rocksdb::ReadOptions());
    bool continue_loop = true;
    for( iter->Seek(key_slice);
        (iter->Valid() && continue_loop);
        iter->Next() )
    {
      rocksdb::Slice key{iter->key()};
      rocksdb::Slice value{iter->value()};
      
      if( ::memcmp(key.data(), key_slice.data(), 16) != 0 )
        break;

      auto item = allocator();
      if( !item ) break;
      
      if( !item->ParseFromArray(value.data(), value.size()) )
      {
        std::string tmp_key = key.ToString();
        LOG_ERROR("Cannot parse value for" << V_(tmp_key));
        continue;
      }
      if( handler(item) )
      {
        to_remove.push_back(key.ToString());
      }
    }
    
    size_t deleted = 0;
    for( auto const & rid : to_remove )
    {
      rocksdb::Status s = impl_->db_->Delete(rocksdb::WriteOptions(), rid);
      if( !s.ok() )
      {
        LOG_ERROR("Failed to remove config" << V_(rid));
      }
      else
      {
        ++deleted;
      }
    }
    
    LOG_TRACE("Deleted" << V_(deleted) << "config out of" << V_(to_remove.size()) << "selected");
    return deleted;
  }
  
  size_t
  store::expire_configs(size_t seconds_range)
  {
    // TODO
    return 0;
  }
  
  size_t
  store::dedup_configs()
  {
    // TODO
    return 0;
  }
  
  void
  store::add_endpoint(const interface::pb::EndpointData & epdata)
  {
    dbid id(epdata);
    std::string key{id.genkey()};
    
    int byte_size = epdata.ByteSize();
    if( byte_size <= 0 )
    {
      LOG_ERROR("empty EndpointData to be written" <<
                M_(epdata) <<
                V_(key) <<
                V_(byte_size));
      THROW_("invalid epdata data");
    }
    
    util::flex_alloc<char, 256> buffer(byte_size);
    
    if( !epdata.SerializeToArray(buffer.get(), byte_size) )
    {
      LOG_ERROR("failed to serialize epdata" <<
                M_(epdata) <<
                V_(key) <<
                V_(byte_size));
      THROW_("failed to serialize epdata");
    }
    
    rocksdb::Slice slice_val(buffer.get(), byte_size);
    
    auto wropts = rocksdb::WriteOptions();
    wropts.sync = true;
    
    rocksdb::Status s = impl_->db_->Put(wropts,
                                        key,
                                        slice_val);
    if( !s.ok() )
    {
      LOG_ERROR("failed to store epdata" <<
                M_(epdata) <<
                V_(key) <<
                V_(byte_size));
      THROW_("failed to store epdata");
    }
  }
  
  bool
  store::get_endpoints(epdata_allocator allocator,
                       epdata_handler handler)
  {
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    size_t n_items = 0;
    interface::pb::EndpointData ep;
    ep.set_name("dummy");
    dbid id(ep);
    std::string seek_key{id.genkey()};
    rocksdb::Slice key_slice{seek_key.c_str(), impl_->prefix_len_};
    
    std::string key_str;

    auto iter = impl_->db_->NewIterator(rocksdb::ReadOptions());
    bool continue_loop = true;
    for( iter->Seek(key_slice);
         (iter->Valid() && continue_loop);
         iter->Next() )
    {
      rocksdb::Slice key{iter->key()};
      rocksdb::Slice value{iter->value()};
      
      if( ::memcmp(key.data(), key_slice.data(), 16) != 0 )
        break;

      auto item = allocator();
      if( !item ) break;
      
      if( !item->ParseFromArray(value.data(), value.size()) )
      {
        key_str = key.ToString();
        LOG_ERROR("Cannot parse value for" << V_(key_str));
        continue;
      }
      continue_loop = handler(item);
      ++n_items;
      if( !continue_loop )
      {
        LOG_TRACE("Stopping endpoint loop because handler returned false");
      }
    }
    
    LOG_TRACE("Read" << V_(n_items) << "endpoints");
    return (n_items > 0);
  }
  
  size_t
  store::remove_endpoints(epdata_allocator allocator,
                          epdata_handler handler)
  {
    if( impl_->db_ == nullptr ) { THROW_("database not initialized"); }
    
    interface::pb::EndpointData ep;
    ep.set_name("dummy");
    dbid id(ep);
    std::string seek_key{id.genkey()};
    rocksdb::Slice key_slice{seek_key.c_str(), impl_->prefix_len_};
    
    std::deque<std::string> to_remove;
    
    auto iter = impl_->db_->NewIterator(rocksdb::ReadOptions());
    bool continue_loop = true;
    for( iter->Seek(key_slice);
        (iter->Valid() && continue_loop);
        iter->Next() )
    {
      rocksdb::Slice key{iter->key()};
      rocksdb::Slice value{iter->value()};
      
      if( ::memcmp(key.data(), key_slice.data(), 16) != 0 )
        break;

      auto item = allocator();
      if( !item ) break;
      
      if( !item->ParseFromArray(value.data(), value.size()) )
      {
        std::string tmp_key = key.ToString();
        LOG_ERROR("Cannot parse value for" << V_(tmp_key));
        continue;
      }
      if( handler(item) )
      {
        to_remove.push_back(key.ToString());
      }
    }
    
    size_t deleted = 0;
    for( auto const & rid : to_remove )
    {
      rocksdb::Status s = impl_->db_->Delete(rocksdb::WriteOptions(), rid);
      if( !s.ok() )
      {
        LOG_ERROR("Failed to remove endpoint" << V_(rid));
      }
      else
      {
        ++deleted;
      }
    }
    
    LOG_TRACE("Deleted" << V_(deleted) << "endpoints out of" << V_(to_remove.size()) << "selected");
    return deleted;
  }
  
  size_t
  store::expire_endpoints(size_t seconds_range)
  {
    // TODO
    return 0;
  }
  
  size_t
  store::dedup_endpoints()
  {
    // TODO
    return 0;
  }
  
  store::store(const std::string & path,
               size_t expiry_seconds)
  : impl_{new impl{path, expiry_seconds}}
  {
  }
  
  store::~store() { }
  
}}
