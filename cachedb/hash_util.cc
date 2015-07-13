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
  hash_util::hash_query(const interface::pb::Query & in,
                        std::string & table_hash_out,
                        colhash_map & column_hashes_out)
  {
    bool ret = true;
    colhash_map tmp_cols_out;
    std::string tmp_tab_out;
    
    XXH64_state_t base_state;
    
    // add everything to the base state first
    
    try
    {
      if( !in.has_table() ) throw 't';
      if( !in.fields_size() ) throw 'f';
        
      if( XXH64_reset(&base_state, 0) != XXH_OK ) throw 'r';
      
      if( in.has_schema() &&
          XXH64_update(&base_state,
                       in.schema().c_str(),
                       in.schema().size()) != XXH_OK ) throw 's';
      
      if( XXH64_update(&base_state,
                         in.table().c_str(),
                         in.table().size()) != XXH_OK ) throw 'x';
      
      // add flter data
      if( in.filter_size() > 0 )
      {
        for( auto & f : in.filter() )
        {
          int byte_size = f.ByteSize();
          if( byte_size > 0 )
          {
            util::flex_alloc<char, 128> buffer(byte_size);
            f.SerializePartialToArray(buffer.get(), byte_size);
            if( XXH64_update(&base_state,
                             buffer.get(),
                             byte_size) != XXH_OK ) throw 'f';
          }
        }
      }
      
      // add limit if defined
      if( in.has_limit() )
      {
        auto limit = in.limit();
        if( XXH64_update(&base_state,
                         &limit,
                         sizeof(limit)) != XXH_OK ) throw 'l';
      }
      
      XXH64_state_t tab_state = base_state;
      
      for( auto const & nm : in.fields() )
      {
        XXH64_state_t field_state = base_state;

        int byte_size = nm.size();
        if( byte_size <= 0 ) throw 'y';
        
        if( XXH64_update(&field_state,
                         nm.c_str(),
                         byte_size ) != XXH_OK ) throw 'u';
        
        if( XXH64_update(&tab_state,
                         nm.c_str(), 
                         byte_size ) != XXH_OK ) throw 'w';
        
        // hash the field
        std::string tmp_col_hash;
        util::hex_util( XXH64_digest(&field_state), tmp_col_hash);
        
        // store the hash value
        tmp_cols_out[nm] = tmp_col_hash;
      }
      
      util::hex_util( XXH64_digest(&tab_state), tmp_tab_out);
      
    }
    catch (char non_important)
    {
      const char txt[2] = { non_important, 0 };
      LOG_ERROR("failed to hash query" <<
                V_(in.queryid()) <<
                V_(in.has_table()) <<
                V_(in.has_schema()) <<
                V_(in.fields_size()) <<
                V_(in.filter_size()) <<
                V_(in.has_limit()) <<
                V_((const char *)txt));
      tmp_tab_out = xxh_error;
      tmp_cols_out.clear();
      ret = false;
    }
    
    // save results
    table_hash_out.swap(tmp_tab_out);
    column_hashes_out.swap(tmp_cols_out);
    return ret;
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

}}
