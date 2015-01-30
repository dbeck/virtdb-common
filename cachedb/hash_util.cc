#include "hash_util.hh"
#include <xxhash.h>
#include <util/flex_alloc.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  namespace
  {
    const char empty_input [] = "= EMPTY  INPUT =";
    const char xxh_error []   = "= XXH    ERROR =";

    const char hex_vals[16] = {
      '0', '1', '2', '3',
      '4', '5', '6', '7',
      '8', '9', 'a', 'b',
      'c', 'd', 'e', 'f'
    };
  }
  
  void
  hash_util::hex(unsigned long long hashed, std::string & res)
  {
    char result[] = "0123456789abcdef";
    
    for( int i=0; i<16; ++i )
    {
      result[i] = hex_vals[(hashed >> (i*4)) & 0x0f];
    }
    res = result;
  }
  
  bool
  hash_util::hash_string(const std::string in,
                        std::string & out)
  {
    bool ret = true;
    if( in.empty() )
    {
      out = empty_input;
      ret = false;
    }
    else
    {
      hex(XXH64(in.c_str(), in.size(), 0), out);
    }
    return ret;
  }
  
  bool
  hash_util::hash_query(const interface::pb::Query & in,
                        std::string & out)
  {
    bool ret = true;
    XXH64_state_t hstate;
    
    try
    {
      if( XXH64_reset(&hstate, 0) != XXH_OK )
        throw 'r';
      
      if( in.has_schema() )
      {
        if( XXH64_update(&hstate,
                         in.schema().c_str(),
                         in.schema().size()) != XXH_OK)
          throw 's';
      }
      
      if( in.has_table() )
      {
        if( XXH64_update(&hstate,
                         in.table().c_str(),
                         in.table().size()) != XXH_OK)
          throw 't';
      }
      else
      {
        throw 'x';
      }
      
      if( in.filter_size() > 0 )
      {
        for( auto & f : in.filter() )
        {
          int byte_size = f.ByteSize();
          if( byte_size > 0 )
          {
            util::flex_alloc<char, 128> buffer(byte_size);
            f.SerializePartialToArray(buffer.get(), byte_size);
            if( XXH64_update(&hstate,
                             buffer.get(),
                             byte_size) != XXH_OK )
              throw 'f';
          }
        }
      }
      
      if( in.has_limit() )
      {
        auto limit = in.limit();
        if( XXH64_update(&hstate,
                         &limit,
                         sizeof(limit)) != XXH_OK )
          throw 'l';
      }
      
      hex( XXH64_digest(&hstate), out);
      
    }
    catch (char non_important)
    {
      const char txt[2] = { non_important, 0 };
      LOG_ERROR("failed to hash query" << M_(in) << V_((const char *)txt));
      out = xxh_error;
      ret = false;
    }
    
    return ret;
  }
  
  bool
  hash_util::hash_field(const interface::pb::Query & q_in,
                        const interface::pb::Field & field_in,
                        std::string & out)
  {
    bool ret = true;
    XXH64_state_t hstate;
    
    try
    {
      if( XXH64_reset(&hstate, 0) != XXH_OK )
        throw 'r';
      
      if( q_in.has_schema() )
      {
        if( XXH64_update(&hstate,
                         q_in.schema().c_str(),
                         q_in.schema().size()) != XXH_OK)
          throw 's';
      }
      
      if( q_in.has_table() )
      {
        if( XXH64_update(&hstate,
                         q_in.table().c_str(),
                         q_in.table().size()) != XXH_OK)
          throw 't';
      }
      else
      {
        throw 'x';
      }
      
      {
        int byte_size = field_in.ByteSize();
        if( byte_size > 0 )
        {
          util::flex_alloc<char, 128> buffer(byte_size);
          field_in.SerializePartialToArray(buffer.get(), byte_size);
          if( XXH64_update(&hstate,
                           buffer.get(),
                           byte_size ) != XXH_OK )
            throw 'f';
        }
        else
        {
          throw 'y';
        }
      }
      
      hex( XXH64_digest(&hstate), out);
    }
    catch (char non_important)
    {
      const char txt[2] = { non_important, 0 };
      LOG_ERROR("failed to hash field" << M_(q_in) << M_(field_in) << V_((const char *)txt));
      out = xxh_error;
      ret = false;
    }
    return ret;
  }
  
}}