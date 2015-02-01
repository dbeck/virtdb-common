#include "cachedb_test.hh"
#include <rocksdb/db.h>
#include <cachedb/store.hh>
#include <cachedb/dbid.hh>
#include <cachedb/hash_util.hh>
#include <memory>
#include <iostream>

using namespace virtdb::test;
using namespace virtdb::cachedb;
using namespace virtdb::interface;
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

TEST_F(CachedbDbIdTest, DateNow)
{
  std::string now;
  dbid::date_now(now);
  EXPECT_FALSE(now.empty());
  EXPECT_EQ(14,now.size());
  bool isnum = now[0] >= '0' && now[0] <= '9';
  EXPECT_TRUE(isnum);
}

TEST_F(CachedbDbIdTest, GenEmpty)
{
  pb::Query         q;
  pb::Config        cfg;
  pb::EndpointData  ep;
  
  {
    auto fun = [&q]() {
      dbid id(q,
              0,
              dbid::clock::now(),
              0);
    };
    EXPECT_THROW(fun(), std::exception);
  }
  
  {
    auto fun = [&cfg]() {
      dbid id(cfg);
    };
    EXPECT_THROW(fun(), std::exception);
  }
  
  {
    auto fun = [&ep]() {
      dbid id(ep);
    };
    EXPECT_THROW(fun(), std::exception);
  }
}

TEST_F(CachedbDbIdTest, GenColumnId)
{
  pb::Query q;
  q.set_table("TEST_TAB");
  auto * f = q.add_fields();
  f->set_name("FIELD1");
  
  std::string res;
  auto now = dbid::clock::now();

  auto fun = [&q,&res,&now]() {
    dbid id(q,
            0,
            now,
            0);
    res = id.genkey();
    
  };
  EXPECT_NO_THROW(fun());
  EXPECT_FALSE(res.empty());
  EXPECT_EQ(64,res.size());
  
  {
    std::string res2 = res;
    f->set_name("FIELD2");
    fun();
    EXPECT_FALSE(res.empty());
    EXPECT_EQ(64,res.size());
    EXPECT_NE(res,res2);
  }

  {
    std::string res2 = res;
    auto * filter = q.add_filter();
    auto * simple = filter->mutable_simple();
    simple->set_variable("VAR");
    filter->set_operand("=");
    simple->set_value("X");
    fun();
    EXPECT_FALSE(res.empty());
    EXPECT_EQ(64,res.size());
    EXPECT_NE(res,res2);
  }

}

TEST_F(CachedbDbIdTest, GenConfigId)
{
  pb::Config cfg;
  cfg.set_name("my component");
  
  std::string res;

  auto fun = [&cfg,&res]() {
    dbid id(cfg,"aabbcccddeeffgg");
    res = id.genkey();
  };
  EXPECT_NO_THROW(fun());
  EXPECT_FALSE(res.empty());
  EXPECT_EQ(res.size(), 64);
}

TEST_F(CachedbDbIdTest, GenEndpointId)
{
  pb::EndpointData  ep;
  ep.set_name("my component");

  std::string res;
  
  auto fun = [&ep,&res]() {
    dbid id(ep,"ggffeedd");
    res = id.genkey();
  };
  EXPECT_NO_THROW(fun());
  EXPECT_FALSE(res.empty());
  EXPECT_EQ(res.size(), 64);
  
  {
    std::string res2 = res;
    ep.set_svctype(pb::ServiceType::IP_DISCOVERY);
    
    EXPECT_NO_THROW(fun());
    EXPECT_FALSE(res.empty());
    EXPECT_EQ(res.size(), 64);
    EXPECT_NE(res,res2);
  }
}

TEST_F(CachedbHashUtilTest, HexFun)
{
  std::string v1;
  hash_util::hex(1, v1);
  EXPECT_FALSE(v1.empty());
  EXPECT_EQ(16, v1.size());
  EXPECT_EQ(v1,"0000000000000001");

  std::string v2;
  hash_util::hex(0x1000200030004000, v2);
  EXPECT_FALSE(v2.empty());
  EXPECT_EQ(16, v2.size());
  EXPECT_EQ(v2,"1000200030004000");
}

TEST_F(CachedbHashUtilTest, HashQuery)
{
  pb::Query q;
  std::string res_t;
  
  EXPECT_FALSE(hash_util::hash_query(q, res_t));
  
  q.set_table("TEST_TAB2");
  
  EXPECT_TRUE(hash_util::hash_query(q, res_t));

  q.set_schema("SCHEMA");
  std::string res_ts;
  EXPECT_TRUE(hash_util::hash_query(q, res_ts));
  
  EXPECT_NE(res_t, res_ts);
  EXPECT_EQ(res_t.size(), res_ts.size());
  
  auto * f= q.add_filter();
  auto * fs = f->mutable_simple();
  fs->set_variable("COL");
  f->set_operand("=");
  fs->set_value("X");

  std::string res_tsf;
  EXPECT_TRUE(hash_util::hash_query(q, res_tsf));
  EXPECT_NE(res_tsf, res_ts);
  EXPECT_EQ(res_tsf.size(), res_ts.size());
}

TEST_F(CachedbHashUtilTest, HashField)
{
  pb::Query q;
  pb::Field fld;
  std::string res_f0,res_f1, res_fl, res_fld;
  
  EXPECT_FALSE(hash_util::hash_field(q, fld, res_f0));
  q.set_table("TABLE");
  EXPECT_FALSE(hash_util::hash_field(q, fld, res_f1));
  EXPECT_FALSE(res_f0.empty());
  EXPECT_EQ(res_f0,res_f1);
  EXPECT_EQ(res_f0.size(), 16);
  
  fld.set_name("FIELD1");
  EXPECT_TRUE(hash_util::hash_field(q, fld, res_fl));
  EXPECT_FALSE(res_fl.empty());
  EXPECT_NE(res_fl,res_f1);
  EXPECT_EQ(res_fl.size(), 16);

  auto * des = fld.mutable_desc();
  des->set_type(pb::Kind::DATE);
  EXPECT_TRUE(hash_util::hash_field(q, fld, res_fld));
  EXPECT_FALSE(res_fld.empty());
  EXPECT_NE(res_fl,res_fld);
  EXPECT_NE(res_f1,res_fld);
  EXPECT_EQ(res_fld.size(), 16);
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

