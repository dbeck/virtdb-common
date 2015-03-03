#pragma once

#include <engine/collector.hh>
#include <util/value_type_reader.hh>
#include <util/timer_service.hh>

namespace virtdb { namespace engine {

  class feeder final
  {
  public:
    typedef util::value_type_reader  vtr;
    typedef std::shared_ptr<feeder>  sptr;
    
  private:
    feeder() = delete;
    feeder(const feeder &) = delete;
    feeder& operator=(const feeder &) = delete;
    
    collector::sptr              collector_;
    int64_t                      act_block_;
    collector::reader_sptr_vec   readers_;
    util::timer_service          timer_svc_;
    
    vtr::status next_block();
    vtr::status get_reader(size_t col_id, vtr::sptr & reader);
    
  public:
    feeder(collector::sptr cll);
    virtual ~feeder();
    
    // value_type_reader interface
    vtr::status read_string(size_t col_id, char ** ptr, size_t & len, bool & null);
    vtr::status read_int32(size_t col_id, int32_t & v, bool & null);
    vtr::status read_int64(size_t col_id, int64_t & v, bool & null);
    vtr::status read_uint32(size_t col_id, uint32_t & v, bool & null);
    vtr::status read_uint64(size_t col_id, uint64_t & v, bool & null);
    vtr::status read_double(size_t col_id, double & v, bool & null);
    vtr::status read_float(size_t col_id, float & v, bool & null);
    vtr::status read_bool(size_t col_id, bool & v, bool & null);
    vtr::status read_bytes(size_t col_id, char ** ptr, size_t & len, bool & null);
  };
  
}}
