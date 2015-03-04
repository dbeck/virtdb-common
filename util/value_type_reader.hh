#pragma once

#include <memory>
#include <google/protobuf/io/coded_stream.h>
#include <vector>
#include <util/exception.hh>
#include <common.pb.h>

namespace virtdb { namespace util {
  
  class value_type_reader
  {
  public:
    typedef std::shared_ptr<value_type_reader> sptr;
    typedef std::unique_ptr<char[]>            buffer;
    
    enum status {
      ok_,
      type_mismatch_,
      end_of_stream_
    };
    
  private:
    value_type_reader();
    value_type_reader(const value_type_reader &) = delete;
    value_type_reader & operator=(const value_type_reader&) = delete;
    
  protected:
    typedef google::protobuf::io::CodedInputStream stream_t;
    
    value_type_reader(buffer && buf, size_t len);
    
    buffer             buffer_;
    stream_t           is_;
    size_t             null_pos_;
    std::vector<bool>  nulls_;
    
  public:
    static sptr construct(buffer && buf, size_t len);
    
    // all input types have a corresponding reader class
    virtual inline status read_string(char ** ptr, size_t & len)  { return type_mismatch_; }
    virtual inline status read_int32(int32_t & v)                 { return type_mismatch_; }
    virtual inline status read_int64(int64_t & v)                 { return type_mismatch_; }
    virtual inline status read_uint32(uint32_t & v)               { return type_mismatch_; }
    virtual inline status read_uint64(uint64_t & v)               { return type_mismatch_; }
    virtual inline status read_double(double & v)                 { return type_mismatch_; }
    virtual inline status read_float(float & v)                   { return type_mismatch_; }
    virtual inline status read_bool(bool & v)                     { return type_mismatch_; }
    virtual inline status read_bytes(char ** ptr, size_t & len)   { return type_mismatch_; }
    
    inline size_t null_pos() const { return null_pos_; }
    inline size_t n_nulls()  const { return nulls_.size(); }
    
    inline bool read_null()
    {
      bool ret = false;
      if( null_pos_ < nulls_.size() )
        ret = nulls_[null_pos_];
      ++null_pos_;
      return ret;
    }
    
    virtual ~value_type_reader() {}
  };
  
  namespace vtr_impl
  {
    template <typename T, uint32_t TAG>
    class packed_reader : public value_type_reader
    {
    protected:
      typedef T    data_t;
      uint32_t     payload_;
      int          endpos_;
      
      template <typename X>
      inline status
      read32(X & v)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          is_.ReadVarint32(&v);
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }

      template <typename X>
      inline status
      read64(X & v)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          is_.ReadVarint64(&v);
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }
      
      inline status
      read(data_t & v)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          is_.ReadRaw(&v, sizeof(data_t));
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }
      
      packed_reader(buffer && buf, size_t len, size_t start_pos)
      : value_type_reader(std::move(buf),len),
        payload_{0},
        endpos_{0}
      {
        is_.Skip(start_pos);
        uint32_t tag = is_.ReadTag();
        if( tag != TAG ) { return; }
        is_.ReadVarint32(&payload_);
        int pos = is_.CurrentPosition();
        endpos_ = pos + payload_;
      }
      
      virtual ~packed_reader() {}
    };
    
    template <uint32_t TAG>
    class buffer_reader : public value_type_reader
    {
      // next tag will be stored in this variable and be read
      // before we pass back the pointer and length. this allows
      // gives the possiblity to the reader to modify it without
      // ruining the parsing.
      //   use-case: add termination zero for strings
      uint32_t next_tag_;
    protected:
      typedef std::pair<char *,size_t> data_t;
      
      virtual inline status
      read(char ** ptr, size_t & rlen)
      {
        if( next_tag_ == TAG )
        {
          uint32_t len = 0;
          is_.ReadVarint32(&len);
          *ptr = buffer_.get()+is_.CurrentPosition();
          rlen = len;
          is_.Skip(len);
          next_tag_ = is_.ReadTag();
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }
      
      buffer_reader(buffer && buf, size_t len, size_t start_pos)
      : value_type_reader(std::move(buf),len),
        next_tag_{0}
      {
        is_.Skip(start_pos);
        next_tag_ = is_.ReadTag();
      }
      
      virtual ~buffer_reader() {}
    };
    
    class string_reader : public buffer_reader<((2<<3)+2)>
    {
    public:
      typedef buffer_reader<((2<<3)+2)> parent_t;
      virtual inline status read_string(char ** ptr, size_t & len) { return read(ptr,len); }
      string_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~string_reader() {}
    };
    
    class int32_reader : public packed_reader<int32_t,((3<<3)+2)>
    {
    public:
      typedef packed_reader<int32_t,((3<<3)+2)> parent_t;
      virtual inline status read_int32(int32_t & v)
      {
        uint32_t n = 0;
        auto ret = read32(n);
        v = n;
        return ret;
      }
      int32_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~int32_reader() {}
    };
    
    class int64_reader : public packed_reader<int64_t,((4<<3)+2)>
    {
    public:
      typedef packed_reader<int64_t,((4<<3)+2)> parent_t;
      virtual inline status read_int64(int64_t & v)
      {
        uint64_t n = 0;
        auto ret = read64(n);
        v = n;
        return ret;
      }
      int64_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~int64_reader() {}
    };
    
    class uint32_reader : public packed_reader<uint32_t,((5<<3)+2)>
    {
    public:
      typedef packed_reader<uint32_t,((5<<3)+2)> parent_t;
      virtual inline status read_uint32(uint32_t & v) { return read32(v); }
      uint32_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~uint32_reader() {}
    };
    
    class uint64_reader : public packed_reader<uint64_t,((6<<3)+2)>
    {
    public:
      typedef packed_reader<uint64_t,((6<<3)+2)> parent_t;
      virtual inline status read_uint64(uint64_t & v) { return read64(v); }
      uint64_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~uint64_reader() {}
    };
    
    class double_reader : public packed_reader<double,((7<<3)+2)>
    {
    public:
      typedef packed_reader<double,((7<<3)+2)> parent_t;
      virtual inline status read_double(double & v) { return read(v); }
      double_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~double_reader() {}
    };
    
    class float_reader : public packed_reader<float,((8<<3)+2)>
    {
    public:
      typedef packed_reader<float,((8<<3)+2)> parent_t;
      virtual inline status read_float(float & v) { return read(v); }
      float_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~float_reader() {}
    };
    
    class bool_reader : public packed_reader<bool,((9<<3)+2)>
    {
    public:
      typedef packed_reader<bool,((9<<3)+2)> parent_t;
      virtual inline status read_bool(bool & v)
      {
        uint32_t v32 = 0;
        auto ret = read32(v32);
        v = (v32 != 0);
        return ret;
      }
      bool_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~bool_reader() {}
    };
    
    class bytes_reader : public buffer_reader<((10<<3)+2)>
    {
    public:
      typedef buffer_reader<((10<<3)+2)> parent_t;
      virtual inline status read_bytes(char ** ptr, size_t & len) { return read(ptr,len); }
      bytes_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
      virtual ~bytes_reader() {}
    };
  }
  
}}
