#include "datasrc_test.hh"
#include <util/active_queue.hh>
#include <thread>
#include <iostream>

using namespace virtdb::util;
using namespace virtdb::test;
using namespace virtdb::datasrc;

TEST_F(PoolTest, Simple)
{
  size_t max_rows{10};
  column::sptr c;
  {
    pool p{max_rows};
    c = p.allocate<int32_column>();
    bool res = p.wait_all_disposed(100);
    EXPECT_FALSE(res);
    c->dispose(std::move(c));
    res = p.wait_all_disposed(100);
    EXPECT_TRUE(res);
    
    c = p.allocate<int32_column>();
    EXPECT_EQ( p.n_allocated(), 1 );
    EXPECT_EQ( p.n_disposed(), 0 );
    
    c->dispose(std::move(c));
    res = p.wait_all_disposed(1);
    EXPECT_TRUE( res );
    EXPECT_EQ( p.n_allocated(), 1 );
    EXPECT_EQ( p.n_disposed(), 1 );
  }
}

TEST_F(PoolTest, Async)
{
  size_t max_rows{10};
  column::sptr c;
  {
    pool p{max_rows};
    c = p.allocate<int32_column>();
    std::thread t([&c](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      c->dispose(std::move(c));
    });
    t.detach();
    bool res = p.wait_all_disposed(1000);
    EXPECT_TRUE(res);
  }
}

TEST_F(PoolTest, AsyncQueued)
{
  size_t max_rows{10};
  active_queue<column::sptr> q{16,[](column::sptr col){
    column::sptr col_copy = col;
    col->dispose(std::move(col_copy));
  }};
  
  {
    pool p{max_rows};
    for( int i=0;i<10000;++i )
    {
      column::sptr c = p.allocate<int32_column>();
      q.push(c);
    }
    bool res = p.wait_all_disposed(1000);
    EXPECT_TRUE(res);
    std::cout << "allocated:" << p.n_allocated() << "\n";
  }
}

TEST_F(PoolTest, AsyncQueuedTripleDispose)
{
  size_t max_rows{10};
  active_queue<column::sptr> q{16,[](column::sptr col){
    if( col ) col->dispose( std::move(col) );
    if( col ) col->dispose( std::move(col) );
    if( col ) col->dispose( std::move(col) );
  }};
  
  {
    pool p{max_rows};
    for( int i=0;i<10000;++i )
    {
      column::sptr c = p.allocate<int32_column>();
      q.push(c);
    }
    bool res = p.wait_all_disposed(1000);
    EXPECT_TRUE(res);
    std::cout << "allocated:" << p.n_allocated() << "\n";
  }
}


