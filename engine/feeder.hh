#pragma once

#include <engine/collector.hh>
#include <util/value_type_reader.hh>
#include <util/timer_service.hh>
#include <util/relative_time.hh>

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
    util::relative_time          timer_;
    std::atomic<uint64_t>        next_block_timeout_ms_;
    
    vtr::status next_block();
    
  public:
    feeder(collector::sptr cll);
    virtual ~feeder();
    
    // value_type_reader interface
    inline vtr::status
    read_string(size_t col_id, char ** ptr, size_t & len, bool & null)
    {
      return readers_[col_id]->read_string(ptr, len, null);
    }
    
    inline vtr::status
    read_int32(size_t col_id, int32_t & v, bool & null)
    {
      return readers_[col_id]->read_int32(v, null);
    }
    
    inline vtr::status
    read_int64(size_t col_id, int64_t & v, bool & null)
    {
      return readers_[col_id]->read_int64(v, null);
    }
    
    inline vtr::status
    read_uint32(size_t col_id, uint32_t & v, bool & null)
    {
      return readers_[col_id]->read_uint32(v, null);
    }
    
    inline vtr::status
    read_uint64(size_t col_id, uint64_t & v, bool & null)
    {
      return readers_[col_id]->read_uint64(v, null);
    }
    
    inline vtr::status
    read_double(size_t col_id, double & v, bool & null)
    {
      return readers_[col_id]->read_double(v, null);
    }
    
    inline vtr::status
    read_float(size_t col_id, float & v, bool & null)
    {
      return readers_[col_id]->read_float(v, null);
    }
    
    inline vtr::status
    read_bool(size_t col_id, bool & v, bool & null)
    {
      return readers_[col_id]->read_bool(v, null);
    }
    
    inline vtr::status
    read_bytes(size_t col_id, char ** ptr, size_t & len, bool & null)
    {
      return readers_[col_id]->read_bytes(ptr, len, null);
    }
    
    inline bool
    has_more() const
    {
      return readers_[0]->has_more();
    }
    
    inline bool
    started() const
    {
      return (act_block_ != -1);
    }
    
    bool fetch_next();
    
    inline void
    next_block_timeout_ms(uint64_t val)
    {
      next_block_timeout_ms_  = val;
    }
    
    inline uint64_t
    next_block_timeout_ms() const
    {
      return next_block_timeout_ms_.load();
    }
    
    const util::relative_time & timer() const;
  };
  
}}
