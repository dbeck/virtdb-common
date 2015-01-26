#include "cachedb_test.hh"
#include <rocksdb/db.h>
#include <memory>
#include <iostream>

using namespace virtdb::test;
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
