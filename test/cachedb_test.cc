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

TEST_F(CachedbHashUtilTest, HashQuery)
{
  // fill query
  pb::Query q;
  q.set_queryid("QueryID");
  q.set_table("KNA1");
  
  // add column: MANDT
  {
    auto * f = q.add_fields();
    f->set_name("MANDT");
    auto * d = f->mutable_desc();
    d->set_type(pb::Kind::STRING);
  }

  // add column: LAND1
  {
    auto * f = q.add_fields();
    f->set_name("LAND1");
    auto * d = f->mutable_desc();
    d->set_type(pb::Kind::NUMERIC);
  }
  
  std::string tab_hash;
  hash_util::colhash_map col_hashes;
  bool res = hash_util::hash_query(q, tab_hash, col_hashes);
  EXPECT_TRUE(res);
  EXPECT_EQ(col_hashes.size(), 2);
  EXPECT_FALSE(tab_hash.empty());
  
  /*
  std::cout << "h0: " << tab_hash << "\n";
  for( const auto & it : col_hashes )
  {
    std::cout << " - " << it.first << " = " << it.second << "\n";
  }
  */
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
  EXPECT_TRUE(d.property("non-existant").empty());
  
  // verify after set
  d.set(c);
  EXPECT_EQ(d.column_set().size(), 1);
  EXPECT_GT(d.len(), 0);
  EXPECT_GT(d.key_len(), 0);
  EXPECT_TRUE(d.property("non-existant").empty());
  EXPECT_FALSE(d.property("data").empty());
  
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
  EXPECT_TRUE(d.property("data").empty());

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

