#include "store.hh"
#include <cachedb/dbid.hh>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/slice_transform.h>
#include <rocksdb/filter_policy.h>
#include <util/exception.hh>
#include <util/flex_alloc.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  struct store::impl
  {
    size_t                           expiry_second_;
    size_t                           prefix_len_;
    int                              bits_per_bloom_key_;
    bool                             block_based_bloom_filter_;
    rocksdb::DB *                    db_;
    rocksdb::Options                 options_;
    rocksdb::BlockBasedTableOptions  table_options_;
    
    impl(size_t expiry_sec)
    : expiry_second_{expiry_sec},
      prefix_len_{expiry_secs_to_prefix_len(expiry_sec)},
      bits_per_bloom_key_{10},
      block_based_bloom_filter_{true},
      db_{nullptr}
    {
      using namespace rocksdb;
      
      // Enable prefix bloom for mem tables
      options_.prefix_extractor.reset(NewFixedPrefixTransform(prefix_len_));
      options_.memtable_prefix_bloom_bits = 100000000;
      options_.memtable_prefix_bloom_probes = 6;
      options_.compression = rocksdb::kLZ4Compression;
      
      // Enable prefix bloom for SST files
      table_options_.filter_policy.reset(NewBloomFilterPolicy(bits_per_bloom_key_,
                                                              block_based_bloom_filter_));
      options_.table_factory.reset(NewBlockBasedTableFactory(table_options_));
      
      Status s = DB::Open(options_, "/tmp/rocksdb",  &db_);
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
    size_t ret = 30;
    if( expiry_seconds > 20       ) --ret;  // YYYYmmddHHMMS- 10s
    if( expiry_seconds > 120      ) --ret;  // YYYYmmddHHMM-- 2m
    if( expiry_seconds > 1200     ) --ret;  // YYYYmmddHHM--- 20m
    if( expiry_seconds > 7200     ) --ret;  // YYYYmmddHH---- 2h
    if( expiry_seconds > 72000    ) --ret;  // YYYYmmddH----- 20h
    if( expiry_seconds > 172800   ) --ret;  // YYYYmmdd------ 2d
    if( expiry_seconds > 1728000  ) --ret;  // YYYYmmd------- 20d
    if( expiry_seconds > 5184000  ) --ret;  // YYYYmm-------- 60d
    if( expiry_seconds > 51840000 ) --ret;  // YYYYm--------- 600d
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
      
    column.name();
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
    
    if( column.SerializeToArray(buffer.get(), byte_size) == false )
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

    rocksdb::Status s = impl_->db_->Put(rocksdb::WriteOptions(),
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
    // TODO
    return false;
  }
  
  size_t
  store::expire_columns(size_t seconds_range)
  {
    // TODO
    return 0;
  }
  
  bool
  store::add_config(const interface::pb::Config & cfg)
  {
    // TODO
    return false;
  }
  
  bool
  store::get_configs(config_allocator allocator,
                     config_handler handler)
  {
    // TODO
    return false;
  }
  
  size_t
  store::remove_configs(config_allocator allocator,
                        config_handler handler)
  {
    // TODO
    return 0;
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
  
  bool
  store::add_endpoint(const interface::pb::EndpointData & epdata)
  {
    // TODO
    return false;
  }
  
  bool
  store::get_endpoints(epdata_allocator allocator,
                       epdata_handler handler)
  {
    // TODO
    return false;
  }
  
  size_t
  store::remove_endpoints(epdata_allocator allocator,
                          epdata_handler handler)
  {
    // TODO
    return 0;
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
  
  store::store(size_t expiry_seconds) : impl_{new impl{expiry_seconds}} {}
  store::~store() { }
  
}}
