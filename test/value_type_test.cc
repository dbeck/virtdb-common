#include "value_type_test.hh"
#include <util/relative_time.hh>
#include <util/value_type_reader.hh>
#include <util/value_type_writer.hh>
#include <util/mempool.hh>
#include <google/protobuf/io/coded_stream.h>
#include <logger.hh>
#include <list>
#include <set>
#include <deque>
#include <vector>
#include <limits>
#include <iostream>
#include <memory>

using namespace virtdb::test;
using namespace virtdb::interface;
using namespace virtdb::util;
using namespace google::protobuf;

namespace
{
  struct measure
  {
    std::string    file_;
    int            line_;
    std::string    function_;
    std::string    msg_;
    relative_time  rt_;
    
    measure(const char * f,
            int l,
            const char * fn,
            const char * msg)
    : file_(f),
    line_(l),
    function_(fn),
    msg_(msg) {}
    
    ~measure()
    {
      double tm = rt_.get_usec() / 1000.0;
      std::cout << "[" << file_ << ':' << line_ << "] " << function_ << "() '" << msg_ << "' " << tm << " ms\n";
    }
  };
  
#define MEASURE_ME(M) measure LOG_INTERNAL_LOCAL_VAR(_m_) { __FILE__, __LINE__, __func__, M };
  
  void fill_int32(pb::ValueType & vt)
  {
    std::vector<int32_t> v;
    v.reserve(1000000);
    for( int i=0; i<1000000; ++i )
      v.push_back(i-500000);
    value_type<int32_t>::set(vt,v.begin(), v.end());
    value_type<std::string>::set_null(vt, 999999);
    value_type<std::string>::set_null(vt, 3);
    ASSERT_EQ(v.size(), 1000000);
  }
  
  void fill_string(pb::ValueType & vt)
  {
    std::vector<std::string> v;
    v.reserve(1000000);
    for( int i=0; i<1000000; ++i )
      v.push_back("Hello World");
    value_type<std::string>::set(vt,v.begin(), v.end());
    value_type<std::string>::set_null(vt, 999999);
    value_type<std::string>::set_null(vt, 3);
    ASSERT_EQ(v.size(), 1000000);
  }
  
  void fill_double(pb::ValueType & vt)
  {
    std::vector<double> v;
    v.reserve(1000000);
    for( int i=0; i<1000000; ++i )
      v.push_back(1.0+i);
    value_type<double>::set(vt,v.begin(), v.end());
    value_type<std::string>::set_null(vt, 999999);
    value_type<std::string>::set_null(vt, 3);
    ASSERT_EQ(v.size(), 1000000);
  }
  
  void print_parts(value_type_writer::sptr wr)
  {
    auto const * parts = wr->get_parts();
    auto const * p = parts;
    while( p )
    {
      std::cout << "allocated:" << p->n_allocated_ << " n:" << p->n_parts_ << "\n";
      for( size_t i=0; i<p->n_parts_; ++i )
      {
        auto const & t = p->parts_[i];
        if( (t.n_used_ + t.n_allocated_) > 0 )
        {
          std::cout << "  head:" << (const void *)t.head_
          << "  data:" << (const void *)t.data_
          << "  used:" << t.n_used_
          << "  allocated:" << t.n_allocated_
          << " [" << i << "]\n";
        }
      }
      p = p->next_;
    }
  }
  
  std::pair< std::unique_ptr<char []>, int>
  copy_parts(value_type_writer::sptr wr)
  {
    int sz = 0;
    {
      auto const * parts = wr->get_parts();
      auto const * p = parts;
      while( p )
      {
        for( size_t i=0; i<p->n_parts_; ++i )
        {
          auto const & t = p->parts_[i];
          if( (t.n_used_ + t.n_allocated_) > 0 )
          {
            sz += t.n_used_;
          }
        }
        p = p->next_;
      }
    }
    std::unique_ptr<char[]> ret{new char[sz]};
    {
      auto const * parts = wr->get_parts();
      auto const * p = parts;
      char * tmp = ret.get();
      while( p )
      {
        for( size_t i=0; i<p->n_parts_; ++i )
        {
          auto const & t = p->parts_[i];
          if( (t.n_used_ + t.n_allocated_) > 0 )
          {
            ::memcpy(tmp, t.data_, t.n_used_);
            tmp += t.n_used_;
          }
        }
        p = p->next_;
      }
    }
    return std::move(std::make_pair(std::move(ret), sz));
  }
  
  
  void print_compare(std::pair<char *, int> baseline,
                     std::pair<char *, int> result)
  {
    int minsz = std::min(baseline.second, result.second);
    int step = 24;
    bool print = false;
    
    for( int i=0; i<minsz; ++i)
    {
      if( baseline.first[i] != result.first[i] )
        print = true;
      
      if( print )
      {
        std::cout << "\nbaseline: -------------- \n  ";
        
        for( int ii=0; ii<step && (i+ii)<minsz; ++ii )
        {
          int b = baseline.first[i+ii];
          char x = baseline.first[i+ii];
          for( int q=0; q<8; ++q )
            std::cout << (((b>>(7-q))&1)==1?"1":"0");
          std::cout << "  [" << i+ii << "] " << x << " \n  ";
        }
        
        std::cout << "\nresult:   -------------- \n  ";
        
        for( int ii=0; ii<step && (i+ii)<minsz; ++ii )
        {
          int r = result.first[i+ii];
          char x = result.first[i+ii];
          for( int q=0; q<8; ++q )
            std::cout << (((r>>(7-q))&1)==1?"1":"0");
          std::cout << "  [" << i+ii << "] " << x << " \n  ";
        }
        
        i+=(step-1);
        print = false;
      }
    }
  }
  
  std::pair< std::unique_ptr<char []>, int >
  int4_baseline()
  {
    MEASURE_ME("Int4 - 1000000 - PB");
    {
      pb::ValueType vt;
      vt.set_type(pb::Kind::INT32);
      for( int32_t i=0; i<1000000; ++i )
      {
        vt.add_int32value(i);
        if( i%3 == 0 )
          value_type_base::set_null(vt, i);
      }
      int message_size = vt.ByteSize();
      std::unique_ptr<char []> buf(new char[message_size]);
      EXPECT_TRUE(vt.SerializeToArray(buf.get(),message_size));
      return std::move(std::make_pair(std::move(buf), message_size));
    }
  }
  
  std::pair< std::unique_ptr<char []>, int >
  date_baseline()
  {
    MEASURE_ME("Date - 1000000 - PB");
    {
      pb::ValueType vt;
      {
        MEASURE_ME("Date - 1000000 - PB - input");
        vt.set_type(pb::Kind::DATE);
        auto mvsv = vt.mutable_stringvalue();
        mvsv->Reserve(1000000);
        for( int32_t i=0; i<1000000; ++i )
        {
          auto * v = vt.add_stringvalue();
          *v = "20150101";
          if( i%3 == 0 )
            value_type_base::set_null(vt, i);
        }
      }
      {
        MEASURE_ME("Date - 1000000 - PB - serialize");
        int message_size = vt.ByteSize();
        std::unique_ptr<char []> buf(new char[message_size]);
        EXPECT_TRUE(vt.SerializeToArray(buf.get(),message_size));
        return std::move(std::make_pair(std::move(buf), message_size));
      }
    }
  }
  
  std::pair< std::unique_ptr<char []>, int>
  string_baseline()
  {
    MEASURE_ME("String - 1000000 - PB");
    {
      pb::ValueType vt;
      {
        MEASURE_ME("String - 1000000 - PB - input");
        vt.set_type(pb::Kind::STRING);
        auto mvsv = vt.mutable_stringvalue();
        mvsv->Reserve(1000000);
        for( int32_t i=0; i<1000000; ++i )
        {
          auto * v = vt.add_stringvalue();
          *v = "Hello World";
          if( i%3 == 0 )
            value_type_base::set_null(vt, i);
        }
      }
      {
        MEASURE_ME("String - 1000000 - PB - serialize");
        int message_size = vt.ByteSize();
        std::unique_ptr<char []> buf(new char[message_size]);
        EXPECT_TRUE(vt.SerializeToArray(buf.get(),message_size));
        return std::move(std::make_pair(std::move(buf), message_size));
      }
    }
  }
}

TEST_F(ValueTypeWriterTest, SimpleInt4_Writer)
{
  auto baseline = int4_baseline();
  value_type_writer::sptr wr;
  {
    MEASURE_ME("Int4 - 1000000 - Writer");
    {
      wr = value_type_writer::construct(pb::Kind::INT32, 1000000);
      auto * w = wr.get();
      {
        MEASURE_ME("Int4 - 1000000 - Writer - Loop");
        for( int32_t i=0; i<1000000; ++i )
        {
          w->write_int32(i);
          if( i%3 == 0 )
            w->set_null(i);
        }
      }
    }
  }
  auto result = copy_parts(wr);
  EXPECT_EQ(baseline.second, result.second);
  if( baseline.second != result.second )
  {
    print_compare(std::make_pair(baseline.first.get(),
                                 baseline.second),
                  std::make_pair(result.first.get(),
                                 result.second));
  }
  EXPECT_EQ(::memcmp(baseline.first.get(),
                     result.first.get(),
                     std::min(baseline.second,
                              result.second)),0);
}

TEST_F(ValueTypeWriterTest, SimpleString_Writer)
{
  auto baseline = string_baseline();
  value_type_writer::sptr wr;
  {
    MEASURE_ME("String - 1000000 - Writer");
    {
      wr = value_type_writer::construct(pb::Kind::STRING, 1000000);
      auto * w = wr.get();
      {
        MEASURE_ME("String - 1000000 - Writer - Loop");
        for( uint32_t i=0; i<1000000; ++i )
        {
          w->write_string(40, [](char * ptr,
                                 size_t sz) {
            ::strncpy(ptr,"Hello World", sz);
            return ::strlen(ptr);
          });
          if( i%3 == 0 )
            w->set_null(i);
        }
      }
    }
  }
  auto result = copy_parts(wr);
  EXPECT_EQ(baseline.second, result.second);
  if( baseline.second != result.second )
    print_compare(std::make_pair(baseline.first.get(),
                                 baseline.second),
                  std::make_pair(result.first.get(),
                                 result.second));
  EXPECT_EQ(::memcmp(baseline.first.get(),
                     result.first.get(),
                     std::min(baseline.second,
                              result.second)),0);
}

TEST_F(ValueTypeWriterTest, SimpleDate_Writer)
{
  auto baseline = date_baseline();
  value_type_writer::sptr wr;
  {
    MEASURE_ME("Date - 1000000 - Writer");
    {
      wr = value_type_writer::construct(pb::Kind::DATE, 1000000);
      auto * w = wr.get();
      {
        MEASURE_ME("Date - 1000000 - Writer - Loop");
        for( uint32_t i=0; i<1000000; ++i )
        {
          w->write_fixlen([](char * ptr) {
            ::memcpy(ptr,"20150101", 8);
            return 8;
          });
          if( i%3 == 0 )
            w->set_null(i);
        }
      }
    }
  }
  auto result = copy_parts(wr);
  EXPECT_EQ(baseline.second, result.second);
  if( baseline.second != result.second )
  {
    print_parts(wr);
    print_compare(std::make_pair(baseline.first.get(),
                                 baseline.second),
                  std::make_pair(result.first.get(),
                                 result.second));
  }
  EXPECT_EQ(::memcmp(baseline.first.get(),
                     result.first.get(),
                     std::min(baseline.second,
                              result.second)),0);
}


TEST_F(ValueTypeReaderTest, Empty)
{
  std::unique_ptr<char[]> buffer;
  auto rdr = value_type_reader::construct(std::move(buffer), 0);
  int32_t v;
  EXPECT_NE(rdr->read_int32(v), value_type_reader::ok_ );
  EXPECT_FALSE(rdr->read_null());
}

TEST_F(ValueTypeReaderTest, Double)
{
  pb::ValueType vt;
  fill_double(vt);
  double check_val = 0;
  
  for( auto const & i : vt.doublevalue() )
    check_val += i;
  
  int buffer_size = vt.ByteSize();
  std::unique_ptr<char[]> buffer{new char[buffer_size]};
  ASSERT_TRUE(vt.SerializeToArray(buffer.get(), buffer_size));
  
  {
    MEASURE_ME("Double baseline - parse only");
    pb::ValueType tmp;
    tmp.ParseFromArray(buffer.get(), buffer_size);
  }
  
  {
    MEASURE_ME("Double coded stream - parse only");
    google::protobuf::io::CodedInputStream is{(const uint8_t *)buffer.get(), buffer_size};
    
    auto tag = is.ReadTag();
    uint32_t typ = 0;
    is.ReadVarint32(&typ);
    EXPECT_EQ(tag, 1<<3);
    EXPECT_TRUE( typ >= 2 && typ <= 18 );
    
    {
      tag = is.ReadTag();
      EXPECT_EQ(tag,((7<<3)+2));
      uint32_t payload = 0;
      is.ReadVarint32(&payload);
      int pos = is.CurrentPosition();
      
      int32_t sval = 0;
      int endpos = pos + payload;
      double n = 0;
      while( pos < endpos )
      {
        is.ReadRaw(&n, sizeof(n));
        pos = is.CurrentPosition();
      }
    }
  }
  
  {
    MEASURE_ME("Double value type reader - parse only");
    auto rdr = value_type_reader::construct(std::move(buffer), buffer_size);
  }
}


TEST_F(ValueTypeReaderTest, String)
{
  pb::ValueType vt;
  fill_string(vt);
  std::string cx{"Hello World"};
  size_t clen = cx.length();
  
  int buffer_size = vt.ByteSize();
  std::unique_ptr<char[]> buffer{new char[buffer_size]};
  ASSERT_TRUE(vt.SerializeToArray(buffer.get(), buffer_size));
  
  std::string s(128,' ');
  std::vector<std::string> preallocated{1000000,s};
  
  {
    MEASURE_ME("String baseline");
    pb::ValueType tmp;
    tmp.ParseFromArray(buffer.get(), buffer_size);
    uint32_t n = 0;
    for( auto const & s : tmp.stringvalue() )
    {
      ++n;
      EXPECT_EQ('H', *s.begin() );
      EXPECT_EQ(s.length(), clen);
    }
    EXPECT_EQ(n, 1000000);
  }
  
  {
    MEASURE_ME("String baseline reserved");
    pb::ValueType tmp;
    auto sv = tmp.mutable_stringvalue();
    std::string* released[1000000];
    sv->Reserve(1000000);
    for( size_t i=0;i<1000000;++i )
      sv->AddAllocated(&preallocated[i]);
    tmp.ParseFromArray(buffer.get(), buffer_size);
    uint32_t n = 0;
    for( auto const & s : tmp.stringvalue() )
    {
      ++n;
      EXPECT_EQ('H', *s.begin() );
      EXPECT_EQ(s.length(), clen);
    }
    EXPECT_EQ(n, 1000000);
    sv = tmp.mutable_stringvalue();
    sv->ExtractSubrange(0, 1000000, released);
  }
  
  {
    MEASURE_ME("String coded stream");
    google::protobuf::io::CodedInputStream is{(const uint8_t *)buffer.get(), buffer_size};
    
    auto tag = is.ReadTag();
    uint32_t typ = 0;
    is.ReadVarint32(&typ);
    EXPECT_EQ(tag, 1<<3);
    EXPECT_TRUE( typ >= 2 && typ <= 18 );
    
    int64_t val = 0;
    
    {
      uint32_t n = 0;
      uint32_t len = 0;
      while( true )
      {
        tag = is.ReadTag();
        if( tag != ((2<<3)+2) ) { break; }
        EXPECT_TRUE(is.ReadVarint32(&len));
        EXPECT_EQ('H', buffer.get()[is.CurrentPosition()]);
        EXPECT_EQ(len, clen);
        is.Skip(len);
        ++n;
      }
      EXPECT_EQ(n, 1000000);
    }
  }
  
  {
    MEASURE_ME("String value type reader");
    auto rdr = value_type_reader::construct(std::move(buffer), buffer_size);
    uint32_t n = 0;
    char * p = nullptr;
    size_t len = 0;
    auto * srdr = dynamic_cast<vtr_impl::string_reader*>(rdr.get());
    while( srdr->string_reader::read_string(&p, len) == value_type_reader::ok_ )
    {
      ++n;
      EXPECT_EQ('H', *p);
      EXPECT_EQ('d', p[len-1]);
      EXPECT_EQ(len, clen);
      p[len] = 0;
    }
    EXPECT_EQ(n, 1000000);
    
    for( size_t i=0; i<rdr->n_nulls(); ++i )
    {
      if( i==999999 || i==3 )
      {
        EXPECT_TRUE( rdr->read_null() );
      }
      else
      {
        EXPECT_FALSE( rdr->read_null() );
      }
    }
    EXPECT_EQ( rdr->n_nulls(), 1000000 );
  }
}


TEST_F(ValueTypeReaderTest, Int32)
{
  pb::ValueType vt;
  fill_int32(vt);
  int64_t check_val = 0;
  
  for( auto const & i : vt.int32value() )
    check_val += i;
  
  int buffer_size = vt.ByteSize();
  std::unique_ptr<char[]> buffer{new char[buffer_size]};
  ASSERT_TRUE(vt.SerializeToArray(buffer.get(), buffer_size));

  {
    MEASURE_ME("Int32 baseline - parse only");
    pb::ValueType tmp;
    tmp.ParseFromArray(buffer.get(), buffer_size);
  }

  {
    MEASURE_ME("Int32 coded stream - parse only");
    google::protobuf::io::CodedInputStream is{(const uint8_t *)buffer.get(), buffer_size};
    
    auto tag = is.ReadTag();
    uint32_t typ = 0;
    is.ReadVarint32(&typ);
    EXPECT_EQ(tag, 1<<3);
    EXPECT_TRUE( typ >= 2 && typ <= 18 );

    {
      tag = is.ReadTag();
      EXPECT_EQ(tag,((3<<3)+2));
      uint32_t payload = 0;
      is.ReadVarint32(&payload);
      int pos = is.CurrentPosition();
      
      int32_t sval = 0;
      int endpos = pos + payload;
      uint32_t n = 0;
      while( pos < endpos )
      {
        is.ReadVarint32(&n);
        sval = (n >> 1) ^ (-(n & 1));
        pos = is.CurrentPosition();
      }
    }
  }
  
  {
    MEASURE_ME("Int32 value type reader - parse only");
    auto rdr = value_type_reader::construct(std::move(buffer), buffer_size);
  }

  buffer.reset(new char[buffer_size]);
  ASSERT_TRUE(vt.SerializeToArray(buffer.get(), buffer_size));

  {
    MEASURE_ME("Int32 baseline - sum");
    pb::ValueType tmp;
    tmp.ParseFromArray(buffer.get(), buffer_size);
    {
      int64_t val = 0;
      for( auto const & i : tmp.int32value() )
        val += i;
      EXPECT_EQ(val, check_val);
    }
  }
  
  {
    MEASURE_ME("Int32 coded stream - sum");
    google::protobuf::io::CodedInputStream is{(const uint8_t *)buffer.get(), buffer_size};
    int64_t val = 0;
    
    auto tag = is.ReadTag();
    uint32_t typ = 0;
    is.ReadVarint32(&typ);
    EXPECT_EQ(tag, 1<<3);
    EXPECT_TRUE( typ >= 2 && typ <= 18 );
    
    {
      tag = is.ReadTag();
      EXPECT_EQ(tag,((3<<3)+2));
      uint32_t payload = 0;
      is.ReadVarint32(&payload);
      int pos = is.CurrentPosition();
      
      int32_t sval = 0;
      int endpos = pos + payload;
      uint32_t n = 0;
      while( pos < endpos )
      {
        is.ReadVarint32(&n);
        sval = (n >> 1) ^ (-(n & 1));
        val += sval;
        pos = is.CurrentPosition();
      }
    }
    
    EXPECT_EQ(val, check_val);
  }
  
  {
    MEASURE_ME("Int32 value type reader - sum");
    auto rdr = value_type_reader::construct(std::move(buffer), buffer_size);
    int64_t val = 0;
    int32_t v = 0;
    while( rdr->read_int32(v) == value_type_reader::ok_ )
      val += v;
    EXPECT_EQ(val, check_val);
  }
  
  buffer.reset(new char[buffer_size]);
  ASSERT_TRUE(vt.SerializeToArray(buffer.get(), buffer_size));

  {
    auto rdr = value_type_reader::construct(std::move(buffer), buffer_size);
    int32_t v = 0;
    int i=0;
    auto * irdr = dynamic_cast<vtr_impl::int32_reader *>(rdr.get());
    while( irdr->int32_reader::read_int32(v) == value_type_reader::ok_ )
    {
      EXPECT_EQ(v, (i-500000));
      ++i;
    }
  }
}

TEST_F(ValueTypeTest, TestString)
{
  typedef std::string val_t;
  typedef uint32_t    other_t;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(99,"empty"), "empty");
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::vector<val_t> values{"apple","peach","lemon"};
  this->set(values.begin(), values.end());

  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_EQ(this->get<val_t>(8,"foo"),"foo");
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,"foo"),"apple");
  EXPECT_EQ(this->get<val_t>(1,"foo"),"peach");
  EXPECT_EQ(this->get<val_t>(2,"foo"),"lemon");
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,99),99);
  EXPECT_EQ(this->get<other_t>(1,99),99);
  EXPECT_EQ(this->get<other_t>(2,99),99);
}

TEST_F(ValueTypeTest, TestI32)
{
  typedef int32_t      val_t;
  typedef std::string  other_t;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(0,-1), -1);

  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::set<val_t> values{1,3,-5,6};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_EQ(this->get<val_t>(8,-3),-3);
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,-9),-5);
  EXPECT_EQ(this->get<val_t>(1,-9),1);
  EXPECT_EQ(this->get<val_t>(2,-9),3);
  EXPECT_EQ(this->get<val_t>(3,-9),6);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,"empty"),"empty");
  EXPECT_EQ(this->get<other_t>(1,"empty"),"empty");
  EXPECT_EQ(this->get<other_t>(2,"empty"),"empty");
  EXPECT_EQ(this->get<other_t>(3,"empty"),"empty");
}

TEST_F(ValueTypeTest, TestI64)
{
  typedef int64_t    val_t;
  typedef double     other_t;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(0,-1), -1);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::list<val_t> values{-6,9,-123};
  this->set(values.begin(), values.end());

  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);

  // non-existant
  EXPECT_EQ(this->get<val_t>(8,-3),-3);
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,-9),-6);
  EXPECT_EQ(this->get<val_t>(1,-9),9);
  EXPECT_EQ(this->get<val_t>(2,-9),-123);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,1.134),1.134);
  EXPECT_EQ(this->get<other_t>(1,1.134),1.134);
  EXPECT_EQ(this->get<other_t>(2,1.134),1.134);
}

TEST_F(ValueTypeTest, TestU32)
{
  typedef uint32_t   val_t;
  typedef bool       other_t;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(0,999), 999);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::deque<val_t> values{98,32,11};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_EQ(this->get<val_t>(8,33),33);
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,19),98);
  EXPECT_EQ(this->get<val_t>(1,19),32);
  EXPECT_EQ(this->get<val_t>(2,19),11);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,false),false);
  EXPECT_EQ(this->get<other_t>(1,false),false);
  EXPECT_EQ(this->get<other_t>(2,false),false);
}

TEST_F(ValueTypeTest, TestU64)
{
  typedef uint64_t  val_t;
  typedef float     other_t;
  
  static const other_t zero = 0.0;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(0,22), 22);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::vector<val_t> values{2,66,89,123};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_EQ(this->get<val_t>(8,33),33);
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,1),2);
  EXPECT_EQ(this->get<val_t>(1,1),66);
  EXPECT_EQ(this->get<val_t>(2,1),89);
  EXPECT_EQ(this->get<val_t>(3,1),123);
  
  // real values, wrong type
  EXPECT_FLOAT_EQ(this->get<other_t>(0,zero),zero);
  EXPECT_FLOAT_EQ(this->get<other_t>(1,zero),zero);
  EXPECT_FLOAT_EQ(this->get<other_t>(2,zero),zero);
  EXPECT_FLOAT_EQ(this->get<other_t>(3,zero),zero);
}

TEST_F(ValueTypeTest, TestDouble)
{
  typedef double    val_t;
  typedef uint64_t  other_t;
  
  static const val_t zero = 0.0;
  
  // get without set
  EXPECT_DOUBLE_EQ(this->get<val_t>(0,zero), zero);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::set<val_t> values{1.34,999.876,123.765};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_DOUBLE_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_DOUBLE_EQ(this->get<val_t>(8,zero),zero);
  
  // real values
  EXPECT_DOUBLE_EQ(this->get<val_t>(0,zero),1.34);
  EXPECT_DOUBLE_EQ(this->get<val_t>(1,zero),123.765);
  EXPECT_DOUBLE_EQ(this->get<val_t>(2,zero),999.876);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,0),0);
  EXPECT_EQ(this->get<other_t>(1,0),0);
  EXPECT_EQ(this->get<other_t>(2,0),0);
}

TEST_F(ValueTypeTest, TestFloat)
{
  typedef float        val_t;
  typedef std::string  other_t;
  
  static const val_t zero = 0.0;

  // get without set
  EXPECT_FLOAT_EQ(this->get<val_t>(0,9.9), 9.9);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);
  
  // setting the values
  std::list<val_t> values{13.4,111.23,998.23};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_FLOAT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_FLOAT_EQ(this->get<val_t>(8,zero),zero);
  
  // real values
  EXPECT_FLOAT_EQ(this->get<val_t>(0,zero),13.4);
  EXPECT_FLOAT_EQ(this->get<val_t>(1,zero),111.23);
  EXPECT_FLOAT_EQ(this->get<val_t>(2,zero),998.23);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,"0"),"0");
  EXPECT_EQ(this->get<other_t>(1,"0"),"0");
  EXPECT_EQ(this->get<other_t>(2,"0"),"0");
}

TEST_F(ValueTypeTest, TestBool)
{
  typedef bool     val_t;
  typedef int32_t  other_t;
  
  // get without set
  EXPECT_EQ(this->get<val_t>(0,false), false);
  
  // size without set
  EXPECT_EQ(this->size<val_t>(), 0);

  // setting the values
  std::deque<val_t> values{true, false, false, true};
  this->set(values.begin(), values.end());
  
  // size after set
  EXPECT_EQ(this->size<val_t>(), values.size());
  
  // size of wrong type
  EXPECT_EQ(this->size<other_t>(), 0);
  
  // non-existant
  EXPECT_EQ(this->get<val_t>(8,true),true);
  
  // real values
  EXPECT_EQ(this->get<val_t>(0,false),true);
  EXPECT_EQ(this->get<val_t>(1,true),false);
  EXPECT_EQ(this->get<val_t>(2,true),false);
  EXPECT_EQ(this->get<val_t>(3,false),true);
  
  // real values, wrong type
  EXPECT_EQ(this->get<other_t>(0,-1),-1);
  EXPECT_EQ(this->get<other_t>(1,-1),-1);
  EXPECT_EQ(this->get<other_t>(2,-1),-1);
  EXPECT_EQ(this->get<other_t>(3,-1),-1);
}

TEST_F(ValueTypeTest, TestNulls)
{
  // value default to non null
  EXPECT_EQ(this->is_null(0), false);

  // setting a null value
  this->set_null<std::string>(2,true);

  // getting from the base class
  EXPECT_EQ(this->is_null(2), true);
  
  // getting from the templated child of a different type
  EXPECT_EQ(this->is_null<double>(2), true);

  // getting the other values
  EXPECT_EQ(this->is_null(0), false);
  EXPECT_EQ(this->is_null(1), false);
  
  // non-existant values are still non-null
  EXPECT_EQ(this->is_null(111), false);
}
