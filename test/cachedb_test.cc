#include "cachedb_test.hh"
#include <rocksdb/db.h>
#include <cachedb/hash_util.hh>
#include <cachedb/column_data.hh>
#include <cachedb/query_column_log.hh>
#include <cachedb/db.hh>
#include <memory>
#include <iostream>
#include <set>

using namespace virtdb::test;
using namespace virtdb::cachedb;
using namespace virtdb::interface;
using namespace rocksdb;

TEST_F(CachedbQueryHasherTest, HashQuery)
{
  pb::Query q;
  std::string tab_hash;
  hash_util::colhash_map col_hashes;
  bool res = hash_util::hash_query(q, tab_hash, col_hashes);
  EXPECT_TRUE(res);
}

TEST_F(CachedbDBTest, InitTests)
{
  {
    column_data       da;
    query_column_log  qcl;
    db                dx;
  
    // set default columns
    da.default_columns();
    qcl.default_columns();
  
    // create storable object vectors
    db::storeable_ptr_vec_t v{&da};
    db::storeable_ptr_vec_t v2{&da, &qcl};
  
    EXPECT_TRUE(dx.init("/tmp/CachedbDBTestInit", v));
    EXPECT_EQ(dx.column_families().size(), da.column_set().size()+1);
    EXPECT_TRUE(dx.init("/tmp/CachedbDBTestInit", v2));
    EXPECT_EQ(dx.column_families().size(), da.column_set().size()+qcl.column_set().size()+1);

    // this must fail. we may call init with more columns, but not less
    EXPECT_FALSE(dx.init("/tmp/CachedbDBTestInit", v));
  }
  system("rm -Rf /tmp/CachedbDBTestInit");
}

namespace
{
  std::string calc_hash(const pb::Column & c)
  {
    column_data d;
    d.set(c);
    return d.key();
  }
  
  size_t calc_len(const pb::Column & c)
  {
    column_data d;
    d.set(c);
    return d.len();
  }
}

TEST_F(CachedbColumnDataTest, DefaultColumns)
{
  column_data d;
  d.default_columns();
  EXPECT_GT(d.column_set().size(), 0);
}

TEST_F(CachedbColumnDataTest, SetClearSet)
{
  pb::Column c;
  {
    auto dta = c.mutable_data();
    dta->set_type(pb::Kind::INT32);
    auto val = dta->mutable_int32value();
    val->Add(1);
  }
  
  column_data d;
  // verify empty data
  EXPECT_TRUE(d.key().empty());
  EXPECT_EQ(d.column_set().size(), 0);
  EXPECT_EQ(d.len(), 0);
  EXPECT_GT(d.key_len(), 0);
  EXPECT_EQ(d.property("non-existant").buffer_, nullptr);
  EXPECT_EQ(d.property("non-existant").len_, 0);
  
  // verify after set
  d.set(c);
  EXPECT_EQ(d.column_set().size(), 1);
  EXPECT_GT(d.len(), 0);
  EXPECT_GT(d.key_len(), 0);
  EXPECT_EQ(d.property("non-existant").buffer_, nullptr);
  EXPECT_EQ(d.property("non-existant").len_, 0);
  EXPECT_NE(d.property("data").buffer_, nullptr);
  EXPECT_GT(d.property("data").len_, 0);
  
  // check second object same values
  column_data d2;
  d2.set(c);
  EXPECT_EQ(d.key(), d2.key());
  EXPECT_EQ(d.column_set(), d2.column_set());

  // clear first object
  d.clear();
  EXPECT_TRUE(d.key().empty());
  EXPECT_EQ(d.column_set().size(), 0);
  EXPECT_EQ(d.len(), 0);
  EXPECT_GT(d.key_len(), 0);
  EXPECT_EQ(d.property("data").buffer_, nullptr);
  EXPECT_EQ(d.property("data").len_, 0);

  // set object back
  d.set(c);
  EXPECT_EQ(d.key(), d2.key());
  EXPECT_EQ(d.column_set(), d2.column_set());
}

TEST_F(CachedbColumnDataTest, Clear3Times)
{
  column_data d;
  d.clear();
  d.clear();
  d.clear();
}

TEST_F(CachedbColumnDataTest, HashColumnData)
{
  pb::Column c;
  std::string hash = calc_hash(c);
  std::string prev = hash;
  size_t prev_len = calc_len(c);

  {
    auto dta = c.mutable_data();
    dta->set_type(pb::Kind::INT32);
    auto val = dta->mutable_int32value();
    val->Add(1);
  }
  
  hash = calc_hash(c);
  EXPECT_EQ(hash,calc_hash(c));
  EXPECT_NE(hash,prev);
  EXPECT_NE(prev_len,calc_len(c));
  prev = hash;
  prev_len = calc_len(c);
  
  {
    c.set_comptype(pb::CompressionType::LZ4_COMPRESSION);
  }
  
  hash = calc_hash(c);
  EXPECT_EQ(hash,calc_hash(c));
  EXPECT_NE(hash,prev);
  EXPECT_NE(prev_len,calc_len(c));
  prev = hash;
  prev_len = calc_len(c);
  
  {
    c.mutable_compresseddata()->assign("Hello");
  }

  hash = calc_hash(c);
  EXPECT_EQ(hash,calc_hash(c));
  EXPECT_NE(hash,prev);
  EXPECT_NE(prev_len,calc_len(c));
  prev = hash;
  prev_len = calc_len(c);
  
  {
    c.set_uncompressedsize(1000);
  }
  
  hash = calc_hash(c);
  EXPECT_EQ(hash,calc_hash(c));
  EXPECT_NE(hash,prev);
  EXPECT_NE(prev_len,calc_len(c));
}

#if 0
TEST_F(XCachedbStoreTest, StoreColumn)
{
  store st("/tmp/StoreColumn",9000);
  
  // fill query
  pb::Query q;
  q.set_queryid("QueryID");
  q.set_table("BSEG");
  {
    auto * f = q.add_fields();
    f->set_name("MANDT");
    auto * d = f->mutable_desc();
    d->set_type(pb::Kind::STRING);
  }
  
  // fill column result
  pb::Column c;
  c.set_queryid("QueryID");
  c.set_name("MANDT");
  {
    auto * d = c.mutable_data();
    d->set_type(pb::Kind::STRING);
    d->add_stringvalue("800");
  }
  c.set_seqno(0);
  c.set_endofdata(true);
  
  std::vector<store::column_sptr> results;
  
  EXPECT_NO_THROW(st.add_column(q, store::clock::now(), c));
  
  auto alloc = []() {
    return store::column_sptr{new pb::Column};
  };
  
  auto get_handler = [&results](store::column_sptr r) {
    results.push_back(r);
    return true;
  };

  EXPECT_TRUE(st.get_columns(q, 0, alloc, get_handler));
  size_t n_results = results.size();
  EXPECT_GT(n_results, 0);
}

TEST_F(XCachedbStoreTest, StoreConfig)
{
  store st("/tmp/StoreConfig",9000);
  pb::Config cfg;
  cfg.set_name("my component");
  EXPECT_NO_THROW(st.add_config(cfg));
  
  std::vector<store::config_sptr> results;
  
  auto alloc = []() {
    return store::config_sptr{new pb::Config};
  };
  
  auto get_handler = [&results](store::config_sptr r) {
    results.push_back(r);
    return true;
  };
  
  auto del_handler = [&results](store::config_sptr r) {
    return true;
  };
  
  EXPECT_TRUE(st.get_configs(alloc, get_handler));
  size_t n_results = results.size();
  size_t deleted = st.remove_configs(alloc, del_handler);
  EXPECT_GT(n_results, 0);
  EXPECT_GT(deleted, 0);
}

TEST_F(XCachedbStoreTest, StoreEndpoint)
{
  store st("/tmp/StoreEndpoint",9000);
  pb::EndpointData ep;
  ep.set_name("my component");
  ep.set_svctype(pb::ServiceType::NONE);
  EXPECT_NO_THROW(st.add_endpoint(ep));
  
  std::vector<store::epdata_sptr> results;
  
  auto alloc = []() {
    return store::epdata_sptr{new pb::EndpointData};
  };
  
  auto get_handler = [&results](store::epdata_sptr r) {
    results.push_back(r);
    return true;
  };

  auto del_handler = [&results](store::epdata_sptr r) {
    return true;
  };
  
  EXPECT_TRUE(st.get_endpoints(alloc, get_handler));
  size_t n_results = results.size();
  size_t deleted = st.remove_endpoints(alloc, del_handler);
  EXPECT_GT(n_results, 0);
  EXPECT_GT(deleted, 0);
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
#endif

