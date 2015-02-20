#include "hash_util.hh"
#include <xxhash.h>
#include <util/flex_alloc.hh>
#include <util/hex_util.hh>
#include <util/exception.hh>
#include <logger.hh>

namespace virtdb { namespace cachedb {
  
  namespace
  {
    const char empty_input [] = "= EMPTY  INPUT =";
    const char xxh_error []   = "= XXH    ERROR =";
  }
  
  bool
  hash_util::hash_query(const interface::pb::Query & query_in,
                        std::string & table_hash_out,
                        colhash_map & column_hashes_out)
  {
    THROW_("implement me");
    return false;
  }
  
  bool
  hash_util::hash_data(const void * p,
                       size_t len,
                       std::string & out)
  {
    bool ret = true;
    if( !p || !len )
    {
      out = empty_input;
      ret = false;
    }
    else
    {
      util::hex_util(XXH64(p, len, 0), out);
    }
    return ret;
  }

#if 0
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
      util::hex_util(XXH64(in.c_str(), in.size(), 0), out);
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
      
      util::hex_util( XXH64_digest(&hstate), out);
      
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
      
      util::hex_util( XXH64_digest(&hstate), out);
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
#endif

}}