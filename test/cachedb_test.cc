#include "cachedb_test.hh"
#include <rocksdb/db.h>
#include <cachedb/hash_util.hh>
#include <cachedb/column_data.hh>
#include <cachedb/query_table_log.hh>

#include <cachedb/db.hh>
#include <memory>
#include <iostream>
#include <set>

using namespace virtdb::test;
using namespace virtdb::cachedb;
using namespace virtdb::interface;
using namespace rocksdb;

TEST_F(CachedbDBTest, QueryTableLogTest)
{
  {
    query_table_log   dl;
    db                cache;
    
    // set default columns
    dl.default_columns();
    
    // create storable object vectors
    db::storeable_ptr_vec_t v{&dl};
    
    EXPECT_TRUE(cache.init("/tmp/CachedbDBTestQueryTableLogTest", v));
    
    auto now = std::chrono::system_clock::now();
    dl.key("0123456789abcdef-mykey");
    dl.data("Hello");
    dl.n_columns(100);
    dl.t0_completed_at(now);
    dl.t0_nblocks(999);
    
    EXPECT_EQ(cache.set(dl), 4);
    
    query_table_log dl2;
    dl2.key(dl.key());
    
    EXPECT_EQ(cache.fetch(dl2), 4);
    EXPECT_EQ(dl2.data(), "Hello");
    EXPECT_EQ(dl2.n_columns(), 100);
    EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(now - dl2.t0_completed_at()).count(), 0);
    EXPECT_EQ(dl2.t0_nblocks(), 999);
    EXPECT_NE(std::chrono::duration_cast<std::chrono::seconds>(now - dl2.t1_completed_at()).count(), 0);
    EXPECT_EQ(dl2.t1_nblocks(), 0);
    
    query_table_log dl3;
    dl3.key(dl.key());
    dl3.column(query_table_log::qn_n_columns); // this has to be there
    dl3.column(query_table_log::qn_t1_nblocks); // this column doesn't exist
    
    EXPECT_EQ(cache.exists(dl3), 1);
  }
  system("rm -Rf /tmp/CachedbDBTestQueryTableLogTest");
}

TEST_F(CachedbStoreableTest, ConvertTimeAndDate)
{
  auto now = std::chrono::system_clock::now();
  storeable::data_t res;
  EXPECT_TRUE(storeable::convert(now, res));
  auto converted = now;
  EXPECT_TRUE(storeable::convert(res,converted));
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(now - converted).count(), 0);
  storeable::data_t res2;
  EXPECT_TRUE(storeable::convert(converted, res2));
  EXPECT_EQ(res,res2);
}

TEST_F(CachedbStoreableTest, ConvertSizeT)
{
  size_t res = 10000;
  std::string str;
  EXPECT_FALSE(storeable::convert("-1", res));
  // check both, that we can convert and haven't modified the value
  EXPECT_TRUE(storeable::convert(res,str));
  EXPECT_EQ(str,"10000");
  size_t res2 = 0;
  EXPECT_TRUE(storeable::convert(str,res2));
  EXPECT_EQ(res,res2);
}

TEST_F(CachedbStoreableTest, ConvertInt64)
{
  int64_t res = 0;
  std::string str;
  EXPECT_TRUE(storeable::convert("-10000", res));
  EXPECT_TRUE(storeable::convert(res,str));
  EXPECT_EQ(str,"-10000");
  int64_t res2 = 0;
  EXPECT_TRUE(storeable::convert(str,res2));
  EXPECT_EQ(res,res2);
}

TEST_F(CachedbHashUtilTest, HashQuery)
{
  // fill query
  pb::Query q;
  q.set_queryid("QueryID");
  q.set_table("KNA1");
  
  // add column: MANDT
  {
    q.add_fields("MANDT");
  }

  // add column: LAND1
  {
    q.add_fields("LAND1");
  }
  
  std::string tab_hash;
  hash_util::colhash_map col_hashes;
  bool res = hash_util::hash_query(q, tab_hash, col_hashes);
  EXPECT_TRUE(res);
  EXPECT_EQ(col_hashes.size(), 2);
  EXPECT_FALSE(tab_hash.empty());
  
  // add a filter
  {
    auto * f = q.add_filter();
    auto * s = f->mutable_simple();
    s->set_variable("MANDT");
    f->set_operand("=");
    s->set_value("800");
  }

  std::string tab_hash2;
  hash_util::colhash_map col_hashes2;
  res = hash_util::hash_query(q, tab_hash2, col_hashes2);
  EXPECT_TRUE(res);
  EXPECT_EQ(col_hashes2.size(), 2);
  EXPECT_FALSE(tab_hash2.empty());
  
  EXPECT_NE(tab_hash, tab_hash2);
  for( auto const & h : col_hashes2 )
  {
    EXPECT_EQ(col_hashes.count(h.first), 1);
    EXPECT_NE(col_hashes[h.first], h.second );
  }
  
  // add limit too
  q.set_limit(100);
  
  std::string tab_hash3;
  hash_util::colhash_map col_hashes3;
  res = hash_util::hash_query(q, tab_hash3, col_hashes3);
  EXPECT_TRUE(res);
  EXPECT_EQ(col_hashes3.size(), 2);
  EXPECT_FALSE(tab_hash3.empty());

  EXPECT_NE(tab_hash2, tab_hash3);
  for( auto const & h : col_hashes3 )
  {
    EXPECT_EQ(col_hashes2.count(h.first), 1);
    EXPECT_NE(col_hashes2[h.first], h.second );
  }
}

TEST_F(CachedbDBTest, InitTests)
{
  {
    column_data       da;
    query_table_log   qtl;
    db                dx;
  
    // set default columns
    da.default_columns();
    qtl.default_columns();
  
    // create storable object vectors
    db::storeable_ptr_vec_t v{&da};
    db::storeable_ptr_vec_t v2{&da, &qtl};
  
    EXPECT_TRUE(dx.init("/tmp/CachedbDBTestInit", v));
    EXPECT_EQ(dx.column_families().size(), da.column_set().size()+1);
    EXPECT_TRUE(dx.init("/tmp/CachedbDBTestInit", v2));
    EXPECT_EQ(dx.column_families().size(), da.column_set().size()+qtl.column_set().size()+1);

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
    auto const & dr = d.data();
    return dr.size();
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
  EXPECT_TRUE(d.property(storeable::qual_name(d,"non-existant")).empty());
  
  // verify after set
  d.set(c);
  EXPECT_EQ(d.column_set().size(), 1);
  EXPECT_GT(d.len(), 0);
  EXPECT_GT(d.key_len(), 0);
  EXPECT_TRUE(d.property(storeable::qual_name(d,"non-existant")).empty());
  EXPECT_FALSE(d.property(storeable::qual_name(d,"data")).empty());
  
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
  EXPECT_TRUE(d.property(storeable::qual_name(d,"data")).empty());

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

