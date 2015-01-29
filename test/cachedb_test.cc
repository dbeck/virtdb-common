#include "cachedb_test.hh"
#include <rocksdb/db.h>
#include <cachedb/store.hh>
#include <cachedb/dbid.hh>
#include <cachedb/hash_util.hh>
#include <memory>
#include <iostream>

using namespace virtdb::test;
using namespace virtdb::cachedb;
using namespace rocksdb;

TEST_F(CachedbStoreTest, SimpleRocksDb)
{
  Options options;
  options.create_if_missing = true;
  
  DB * db = nullptr;
  std::shared_ptr<DB> db_ptr;
  
  Status s = DB::Open(options, "/tmp/CachedbStoreTest",  &db);
  if( s.ok() && db )
  {
    db_ptr.reset(db);
    auto put_res = db_ptr->Put(WriteOptions(), "hello", "world" );
    EXPECT_TRUE( put_res.ok() );
    
    std::string res;
    auto get_res = db_ptr->Get(ReadOptions(), "hello", &res );
    EXPECT_TRUE(get_res.ok());
    EXPECT_EQ( res, "world" );
    
    auto del_res = db_ptr->Delete(WriteOptions(), "hello");
    EXPECT_TRUE(del_res.ok());    
  }
  else
  {
    std::cerr << "failed to open rocks DB";
  }
}

TEST_F(CachedbDbIdTest, GenColumnId)
{
}

TEST_F(CachedbDbIdTest, GenConfigId)
{
}

TEST_F(CachedbDbIdTest, GenEndpointId)
{
}

TEST_F(CachedbHashUtilTest, HashQuery)
{
}

TEST_F(CachedbHashUtilTest, HashField)
{
}

TEST_F(CachedbHashUtilTest, HashString)
{
  std::string hello;
  EXPECT_TRUE(hash_util::hash_string("hello", hello));
  EXPECT_FALSE(hello.empty());
  EXPECT_EQ(16, hello.size());

  {
    std::string tmp;
    EXPECT_TRUE(hash_util::hash_string("hello", tmp));
    EXPECT_EQ(tmp, hello);
  }

  {
    std::string tmp;
    EXPECT_TRUE(hash_util::hash_string("hello1", tmp));
    EXPECT_NE(tmp, hello);
  }
}

