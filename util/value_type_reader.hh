#pragma once

#include <util/exception.hh>
#include <common.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <memory>
#include <vector>

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
    
    buffer                        buffer_;
    stream_t                      is_;
    size_t                        null_pos_;
    size_t                        n_nulls_;
    std::unique_ptr<uint32_t[]>   nulls_;
    
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
    virtual inline bool   has_more() const                        { return false;          }
    
    inline size_t null_pos() const { return null_pos_; }
    inline size_t n_nulls()  const { return n_nulls_; }
    
    inline bool read_null()
    {
      bool ret = false;
      if( null_pos_ < n_nulls_ )
      {
        uint32_t p = nulls_.get()[null_pos_/32];
        ret = (((p>>(null_pos_&(31)))&1) == 1);
      }
      ++null_pos_;
      return ret;
    }
    
    // return the null value too
    // all input types have a corresponding reader class
    virtual inline status read_string(char ** ptr, size_t & len, bool & null)  { return type_mismatch_; }
    virtual inline status read_int32(int32_t & v, bool & null)                 { return type_mismatch_; }
    virtual inline status read_int64(int64_t & v, bool & null)                 { return type_mismatch_; }
    virtual inline status read_uint32(uint32_t & v, bool & null)               { return type_mismatch_; }
    virtual inline status read_uint64(uint64_t & v, bool & null)               { return type_mismatch_; }
    virtual inline status read_double(double & v, bool & null)                 { return type_mismatch_; }
    virtual inline status read_float(float & v, bool & null)                   { return type_mismatch_; }
    virtual inline status read_bool(bool & v, bool & null)                     { return type_mismatch_; }
    virtual inline status read_bytes(char ** ptr, size_t & len, bool & null)   { return type_mismatch_; }
    
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
      read32(X & v, bool & null)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          null = read_null();
          is_.ReadVarint32(&v);
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }
      
      inline bool has_more() const
      {
        return ( is_.CurrentPosition() < endpos_ );
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

      template <typename X>
      inline status
      read64(X & v, bool & null)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          is_.ReadVarint64(&v);
          null = read_null();
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

      inline status
      read(data_t & v, bool & null)
      {
        int pos = is_.CurrentPosition();
        if( pos < endpos_ )
        {
          is_.ReadRaw(&v, sizeof(data_t));
          null = read_null();
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

      virtual inline status
      read(char ** ptr, size_t & rlen, bool & null)
      {
        if( next_tag_ == TAG )
        {
          uint32_t len = 0;
          is_.ReadVarint32(&len);
          *ptr = buffer_.get()+is_.CurrentPosition();
          rlen = len;
          is_.Skip(len);
          next_tag_ = is_.ReadTag();
          null = read_null();
          return ok_;
        }
        else
        {
          return end_of_stream_;
        }
      }
      
      inline bool
      has_more() const
      {
        return ( next_tag_ == TAG );
      }
      
      inline
      buffer_reader(buffer && buf, size_t len, size_t start_pos)
      : value_type_reader(std::move(buf),len),
        next_tag_{0}
      {
        is_.Skip(start_pos);
        next_tag_ = is_.ReadTag();
      }
      
      virtual ~buffer_reader() {}
    };
    
    // concrete type readers constructed by construct() in value_type_reader
    
    class string_reader : public buffer_reader<((2<<3)+2)>
    {
      typedef buffer_reader<((2<<3)+2)> parent_t;
      friend class value_type_reader;
      inline string_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_string(char ** ptr, size_t & len) { return read(ptr,len); }
      virtual inline status read_string(char ** ptr, size_t & len, bool & null) { return read(ptr,len, null); }
      virtual ~string_reader() {}
    };
    
    class int32_reader : public packed_reader<int32_t,((3<<3)+2)>
    {
      typedef packed_reader<int32_t,((3<<3)+2)> parent_t;
      friend class value_type_reader;
      inline int32_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_int32(int32_t & v)
      {
        uint32_t n = 0;
        auto ret = read32(n);
        v = n;
        return ret;
      }
      virtual inline status read_int32(int32_t & v, bool & null)
      {
        uint32_t n = 0;
        auto ret = read32(n, null);
        v = n;
        return ret;
      }
      virtual ~int32_reader() {}
    };
    
    class int64_reader : public packed_reader<int64_t,((4<<3)+2)>
    {
      typedef packed_reader<int64_t,((4<<3)+2)> parent_t;
      friend class value_type_reader;
      inline int64_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_int64(int64_t & v)
      {
        uint64_t n = 0;
        auto ret = read64(n);
        v = n;
        return ret;
      }
      virtual inline status read_int64(int64_t & v, bool & null)
      {
        uint64_t n = 0;
        auto ret = read64(n, null);
        v = n;
        return ret;
      }
      virtual ~int64_reader() {}
    };
    
    class uint32_reader : public packed_reader<uint32_t,((5<<3)+2)>
    {
      typedef packed_reader<uint32_t,((5<<3)+2)> parent_t;
      friend class value_type_reader;
      inline uint32_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_uint32(uint32_t & v) { return read32(v); }
      virtual inline status read_uint32(uint32_t & v, bool & null) { return read32(v,null); }
      virtual ~uint32_reader() {}
    };
    
    class uint64_reader : public packed_reader<uint64_t,((6<<3)+2)>
    {
      typedef packed_reader<uint64_t,((6<<3)+2)> parent_t;
      friend class value_type_reader;
      inline uint64_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_uint64(uint64_t & v) { return read64(v); }
      virtual inline status read_uint64(uint64_t & v, bool & null) { return read64(v,null); }
      virtual ~uint64_reader() {}
    };
    
    class double_reader : public packed_reader<double,((7<<3)+2)>
    {
      typedef packed_reader<double,((7<<3)+2)> parent_t;
      friend class value_type_reader;
      inline double_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_double(double & v) { return read(v); }
      virtual inline status read_double(double & v, bool & null) { return read(v,null); }
      virtual ~double_reader() {}
    };
    
    class float_reader : public packed_reader<float,((8<<3)+2)>
    {
      typedef packed_reader<float,((8<<3)+2)> parent_t;
      friend class value_type_reader;
      inline float_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_float(float & v) { return read(v); }
      virtual inline status read_float(float & v, bool & null) { return read(v,null); }
      virtual ~float_reader() {}
    };
    
    class bool_reader : public packed_reader<bool,((9<<3)+2)>
    {
      typedef packed_reader<bool,((9<<3)+2)> parent_t;
      friend class value_type_reader;
      inline bool_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_bool(bool & v)
      {
        uint32_t v32 = 0;
        auto ret = read32(v32);
        v = (v32 != 0);
        return ret;
      }
      virtual inline status read_bool(bool & v, bool & null)
      {
        uint32_t v32 = 0;
        auto ret = read32(v32, null);
        v = (v32 != 0);
        return ret;
      }
      virtual ~bool_reader() {}
    };
    
    class bytes_reader : public buffer_reader<((10<<3)+2)>
    {
      typedef buffer_reader<((10<<3)+2)> parent_t;
      friend class value_type_reader;
      inline bytes_reader(buffer && buf, size_t len, size_t start_pos) : parent_t(std::move(buf),len,start_pos) {}
    public:
      virtual inline status read_bytes(char ** ptr, size_t & len) { return read(ptr,len); }
      virtual inline status read_bytes(char ** ptr, size_t & len, bool & null) { return read(ptr,len,null); }
      virtual ~bytes_reader() {}
    };
  }
  
}}
