#include <util/value_type_reader.hh>
#include <util/exception.hh>
#include <common.pb.h>

namespace virtdb { namespace util {
  
  value_type_reader::value_type_reader()
  : is_(nullptr, 0), null_pos_{0}, n_nulls_{0}
  {
  }
  
  value_type_reader::value_type_reader(buffer && buf,
                                       size_t len)
  : buffer_{std::move(buf)},
    is_((uint8_t *)buffer_.get(), len),
    null_pos_{0},
    n_nulls_{0}
  {
  }

  value_type_reader::sptr
  value_type_reader::construct(buffer && buf, size_t len)
  {
    value_type_reader::sptr ret;
    if( buf.get() == nullptr || len == 0 )
    {
      ret.reset(new value_type_reader);
      return ret;
    }
    
    std::unique_ptr<uint32_t[]> tmp_nulls;
    size_t n_nulls = 0;
    size_t start_pos = 0;
    
    uint32_t typ = 0;
    {
      // construct a temporary stream
      stream_t tmp_is((uint8_t *)buf.get(), len);
      
      // read the type field's tag
      auto tag = tmp_is.ReadTag();
      
      // read the type field
      tmp_is.ReadVarint32(&typ);
      
      // check tag and type
      if( tag != 1<<3 )
      {
        THROW_("invalid buffer, bad tag");
      }
      
      if( typ < 2 || typ > 18 )
      {
        THROW_("invalid buffer, bad typ");
      }
      
      bool null_ok = false;
      
      auto read_nulls = [&]() {
        if( null_ok ) return;
        uint32_t payload = 0;
        bool rv = tmp_is.ReadVarint32(&payload);
        if( rv )
        {
          size_t null_size = 1+((payload+31)/32);
          tmp_nulls.reset(new uint32_t[null_size]);
          uint32_t * p = tmp_nulls.get();
          ::memset(tmp_nulls.get(),0,(null_size*sizeof(uint32_t)));
          
          int pos = tmp_is.CurrentPosition();
          int endpos = pos + payload;
          while( pos < endpos )
          {
            uint32_t val = 0;
            tmp_is.ReadVarint32(&val);
            pos = tmp_is.CurrentPosition();
            if( val )
            {
              uint32_t * px = p+(n_nulls/32);
              uint32_t nval = (1<<(n_nulls&31));
              *px = *px | nval;
            }
            ++n_nulls;
          }
        }
        null_ok = true;
      };
      
      start_pos = tmp_is.CurrentPosition();
      
      // next tag is either null's or ours
      tag = tmp_is.ReadTag();
      
      // null tag came first
      if( tag == ((11<<3)+2) )
      {
        read_nulls();
        start_pos = tmp_is.CurrentPosition();
        tag = tmp_is.ReadTag();
      }
      // bytes or strings
      else if( tag == ((2<<3)+2) || tag == ((10<<3)+2) )
      {
        uint32_t ltag = tag;
        while( true )
        {
          if( tag != ltag ) { break; }
          uint32_t len = 0;
          tmp_is.ReadVarint32(&len);
          tmp_is.Skip(len);
          tag = tmp_is.ReadTag();
        }
      }
      // packed otherwise
      else
      {
        uint32_t payload = 0;
        tmp_is.ReadVarint32(&payload);
        tmp_is.Skip(payload);
        tag = tmp_is.ReadTag();
      }
      
      if( tag == ((11<<3)+2) )
      {
        read_nulls();
      }
    }
    
    switch( typ )
    {
      case interface::pb::Kind::STRING:
      case interface::pb::Kind::DATE:
      case interface::pb::Kind::TIME:
      case interface::pb::Kind::DATETIME:
      case interface::pb::Kind::NUMERIC:
      case interface::pb::Kind::INET4:
      case interface::pb::Kind::INET6:
      case interface::pb::Kind::MAC:
      case interface::pb::Kind::GEODATA:
        ret.reset(new vtr_impl::string_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::INT32:
        ret.reset(new vtr_impl::int32_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::INT64:
        ret.reset(new vtr_impl::int64_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::BOOL:
        ret.reset(new vtr_impl::bool_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::UINT32:
        ret.reset(new vtr_impl::uint32_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::UINT64:
        ret.reset(new vtr_impl::uint64_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::DOUBLE:
        ret.reset(new vtr_impl::double_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::FLOAT:
        ret.reset(new vtr_impl::float_reader(std::move(buf),len,start_pos));
        break;
        
      case interface::pb::Kind::BYTES:
        ret.reset(new vtr_impl::bytes_reader(std::move(buf),len,start_pos));
        break;
    };
    
    ret->nulls_.swap(tmp_nulls);
    ret->n_nulls_ = n_nulls;
    return ret;
  }
  
}}
