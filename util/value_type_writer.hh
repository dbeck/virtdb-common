#pragma once

#include <util/exception.hh>
#include <util/mempool.hh>
#include <common.pb.h>
#include <google/protobuf/io/coded_stream.h>
#include <memory>
#include <vector>
#include <functional>

namespace virtdb { namespace util {
  
  class value_type_writer
  {
  public:
    typedef std::shared_ptr<value_type_writer> sptr;
    
    struct part
    {
      uint8_t *   head_;
      size_t      n_allocated_;
      uint8_t *   data_;
      size_t      n_used_;
      
      inline void
      allocate(mempool & pool, size_t size)
      {
        head_        = pool.allocate<uint8_t>(size);
        n_allocated_ = size;
        data_        = head_;
        n_used_      = 0;
      }
    };
    
    struct part_chain
    {
      part *        parts_;
      size_t        n_parts_;
      size_t        n_allocated_;
      part_chain *  next_;
      
      inline void
      allocate(mempool & pool, size_t size)
      {
        parts_        = pool.allocate<part>(size);
        n_parts_      = 0;
        n_allocated_  = size;
        next_         = 0;
        ::memset(parts_,0,sizeof(part)*size);
      }
    };
    
  protected:
    mempool             mpool_;
    part_chain          root_;
    part_chain          nulls_;
    const size_t        estimated_item_count_;
    size_t              last_null_;
    part_chain *        null_part_chain_;
    part *              null_part_;
    size_t              null_offset_;
    
    value_type_writer() = delete;
    value_type_writer& operator=(const value_type_writer &) = delete;
    value_type_writer(const value_type_writer &) = delete;
    
    value_type_writer(interface::pb::Kind kind,
                      size_t initial_mempool_size,
                      size_t estimated_item_count);
    
  public:
    static sptr construct(interface::pb::Kind kind,
                          size_t estimated_item_count);
    
    virtual ~value_type_writer() {}
    
    inline const part_chain * get_parts() const { return &root_; }
    
    // string and bytes type tell how much space they need
    // and the passed function will receive the allocated buffer.
    // the 'fun' function returns the actual size used, so writer
    // can tell mempool to reuse
    virtual inline void write_string(size_t desired_size,
                                     std::function<size_t(char *,
                                                          size_t)> fun)  { }
    virtual inline void write_fixlen(std::function<size_t(char *)> fun)  { }
    virtual inline void write_bytes(size_t desired_size,
                                    std::function<size_t(char *,
                                                         size_t)> fun)   { }
    
    virtual inline void write_int32(int32_t v)    { }
    virtual inline void write_int64(int64_t v)    { }
    virtual inline void write_uint32(uint32_t v)  { }
    virtual inline void write_uint64(uint64_t v)  { }
    virtual inline void write_double(double v)    { }
    virtual inline void write_float(float v)      { }
    virtual inline void write_bool(bool v)        { }
    
    inline void
    allocate_more_null_area(size_t requested_space)
    {
      size_t prev_alloc    = null_part_->n_allocated_;
      size_t to_allocate   = prev_alloc;
      
      if( null_part_chain_->n_parts_ == null_part_chain_->n_allocated_ )
      {
        // we have eaten up all space in the current part_chain, will need a new part_chain
        auto * new_chain = mpool_.allocate<part_chain>(1);
        new_chain->allocate(mpool_,8);
        
        auto old_chain    = null_part_chain_;
        null_part_chain_  = new_chain;
        null_part_         = new_chain->parts_;
        
        // chain this in
        null_part_chain_->n_parts_  = 1;
        null_part_chain_->next_     = old_chain->next_;
        old_chain->next_            = null_part_chain_;
      }
      else
      {
        // we have empty slot in the actual part chain
        auto pos = null_part_chain_->n_parts_;
        ++(null_part_chain_->n_parts_);
        null_part_ = null_part_chain_->parts_ + pos;
      }
      
      if( to_allocate < requested_space )
      {
        // we over-allocate for the case when estimated size is completely wrong
        to_allocate = requested_space + 256;
      }
      
      // finally allocate space and update free space and buffer
      null_part_->allocate(mpool_, to_allocate);
      null_offset_ += prev_alloc;
    }
    
    inline void set_null(size_t pos)
    {
      using namespace google::protobuf::io;
      
      size_t act_pos = pos - null_offset_;
      if( act_pos < null_part_->n_allocated_ )
      {
        if( act_pos >= null_part_->n_used_ )
          null_part_->n_used_ = act_pos+1;
        null_part_->data_[act_pos] = 1;
        
        if( pos > last_null_ )
          last_null_ = (pos+1);
        
        uint8_t * payload_end = CodedOutputStream::WriteVarint32ToArray(last_null_, nulls_.parts_[0].data_+1);
        size_t n_used = payload_end-nulls_.parts_[0].data_;
        nulls_.parts_[0].n_used_ = n_used;
        return;
      }
      
      allocate_more_null_area(act_pos+1);
      set_null(pos);
    }
  };
  
  namespace vtw_impl
  {
    template <typename T, uint32_t TAG, interface::pb::Kind KIND>
    class packed_writer : public value_type_writer
    {
    protected:
      typedef T     data_t;
      const size_t  max_item_size_;
      uint8_t *     payload_start_;
      part_chain *  act_part_chain_;
      part *        act_part_;
      uint8_t *     act_buffer_;
      size_t        act_free_bytes_;
      size_t        payload_;
      
      inline packed_writer(size_t estimated_item_count)
      : value_type_writer(KIND,
                          (( ((((sizeof(T)*8)+7)/7)) +2)*estimated_item_count),
                          estimated_item_count),
        max_item_size_{(((sizeof(T)*8)+7)/7)},
        payload_start_{nullptr},
        act_part_chain_{&root_},
        act_part_{nullptr},
        act_buffer_{nullptr},
        act_free_bytes_{0},
        payload_{0}
      {
        using namespace google::protobuf::io;
        
        root_.allocate(mpool_,8);
        root_.next_ = &nulls_;
        // type                             // (1
        // XValue { Tag, PayLoad, Data* }   //  1, 1) 2->
        // IsNull { Tag, PayLoad, Data* }   //  3, 4->
        root_.parts_[0].allocate(mpool_, 16);
        root_.parts_[1].allocate(mpool_, (max_item_size_*estimated_item_count));
        
        // fill first buffer
        {
          uint8_t * spos = root_.parts_[0].data_;
          spos = CodedOutputStream::WriteTagToArray(1<<3, spos);
          spos = CodedOutputStream::WriteVarint32ToArray(KIND, spos);
          spos = CodedOutputStream::WriteTagToArray(TAG, spos);
          payload_start_ = spos;
          root_.parts_[0].n_used_ = spos-root_.parts_[0].head_;
          ++(root_.n_parts_);
        }
        
        // setup write area
        {
          act_part_         = &(act_part_chain_->parts_[1]);
          act_buffer_       = act_part_->head_;
          act_free_bytes_   = act_part_->n_allocated_;
          ++(root_.n_parts_);
        }
      }
      
      inline void
      allocate_more_write_area()
      {
        size_t last_alloc = act_part_->n_allocated_;
        
        if( act_part_chain_->n_parts_ == act_part_chain_->n_allocated_ )
        {
          // we have eaten up all space in the current part_chain, will need a new part_chain
          auto * new_chain = mpool_.allocate<part_chain>(1);
          new_chain->allocate(mpool_,8);
          
          auto old_chain = act_part_chain_;
          act_part_chain_   = new_chain;
          act_part_         = new_chain->parts_;
          
          // chain this in
          act_part_chain_->n_parts_  = 1;
          act_part_chain_->next_     = old_chain->next_;
          old_chain->next_           = act_part_chain_;
        }
        else
        {
          // we have empty slot in the actual part chain
          auto pos = act_part_chain_->n_parts_;
          ++(act_part_chain_->n_parts_);
          act_part_ = act_part_chain_->parts_ + pos;
        }
        
        // finally allocate space and update free space and buffer
        act_part_->allocate(mpool_, last_alloc);
        act_buffer_       = act_part_->head_;
        act_free_bytes_   = act_part_->n_allocated_;
      }

      inline void
      write32(uint32_t v)
      {
        using namespace google::protobuf::io;
        
        // have enough space, do the write
        if( act_free_bytes_ >= max_item_size_ )
        {
          auto * prev_buffer = act_buffer_;
          act_buffer_ = CodedOutputStream::WriteVarint32ToArray(v, act_buffer_);
          uint32_t written = act_buffer_-prev_buffer;
          act_free_bytes_ -= written;
          payload_ += written;
          act_part_->n_used_ += written;
          // update payload
          auto * res = CodedOutputStream::WriteVarint32ToArray(payload_, payload_start_);
          root_.parts_[0].n_used_ = res-root_.parts_[0].head_;
          return;
        }
        
        allocate_more_write_area();
        write32(v);
      }
      
      inline void
      write64(uint64_t v)
      {
        using namespace google::protobuf::io;
        
        // have enough space, do the write
        if( act_free_bytes_ >= max_item_size_ )
        {
          auto * prev_buffer = act_buffer_;
          act_buffer_ = CodedOutputStream::WriteVarint64ToArray(v, act_buffer_);
          uint32_t written = act_buffer_-prev_buffer;
          act_free_bytes_ -= written;
          payload_ += written;
          act_part_->n_used_ += written;
          // update payload
          auto * res = CodedOutputStream::WriteVarint32ToArray(payload_, payload_start_);
          root_.parts_[0].n_used_ = res-root_.parts_[0].head_;
          return;
        }
        
        allocate_more_write_area();
        write32(v);
      }
      
      inline void
      write(data_t v)
      {
        using namespace google::protobuf::io;
        
        // have enough space, do the write
        if( act_free_bytes_ >= max_item_size_ )
        {
          auto * prev_buffer = act_buffer_;
          act_buffer_ = CodedOutputStream::WriteRawToArray(&v, sizeof(v), act_buffer_);
          uint32_t written = act_buffer_-prev_buffer;
          act_free_bytes_ -= written;
          payload_ += written;
          act_part_->n_used_ += written;
          // update payload
          auto * res = CodedOutputStream::WriteVarint32ToArray(payload_, payload_start_);
          root_.parts_[0].n_used_ = res-root_.parts_[0].head_;
          return;
        }
        
        allocate_more_write_area();
        write32(v);
      }
    };

    template <uint32_t TAG, interface::pb::Kind KIND, size_t LEN>
    class fixlen_writer : public value_type_writer
    {
    protected:
      part_chain *  act_part_chain_;
      part *        act_part_;
      uint8_t *     act_buffer_;
      size_t        act_free_bytes_;
      
      inline fixlen_writer(size_t estimated_item_count)
      : value_type_writer(KIND,
                          ((LEN+16)*estimated_item_count),
                          estimated_item_count),
        act_part_chain_{&root_},
        act_part_{nullptr},
        act_buffer_{nullptr},
        act_free_bytes_{0}
      {
        using namespace google::protobuf::io;
        
        root_.allocate(mpool_,8);
        root_.next_ = &nulls_;
        
        // type                            // 1
        // XValue { Tag, Size, Data } *    // 2
        root_.parts_[0].allocate(mpool_, 1);
        root_.parts_[1].allocate(mpool_, ((LEN+8)*estimated_item_count));
        
        // fill first buffer : the type
        {
          uint8_t * spos = root_.parts_[0].data_;
          spos = CodedOutputStream::WriteTagToArray(1<<3, spos);
          spos = CodedOutputStream::WriteVarint32ToArray(KIND, spos);
          root_.parts_[0].n_used_ = (spos-root_.parts_[0].data_);
          ++(root_.n_parts_);
        }
        
        // setup write area
        {
          act_part_         = &(act_part_chain_->parts_[1]);
          act_buffer_       = act_part_->head_;
          act_free_bytes_   = act_part_->n_allocated_;
          ++(root_.n_parts_);
        }
      }
      
      inline void
      allocate_more_write_area()
      {
        size_t last_alloc = act_part_->n_allocated_;
        
        if( act_part_chain_->n_parts_ == act_part_chain_->n_allocated_ )
        {
          // we have eaten up all space in the current part_chain, will need a new part_chain
          auto * new_chain = mpool_.allocate<part_chain>(1);
          new_chain->allocate(mpool_,8);
          
          auto old_chain    = act_part_chain_;
          act_part_chain_   = new_chain;
          act_part_         = new_chain->parts_;
          
          // chain this in
          act_part_chain_->n_parts_  = 1;
          act_part_chain_->next_     = old_chain->next_;
          old_chain->next_           = act_part_chain_;
        }
        else
        {
          // we have empty slot in the actual part chain
          auto pos = act_part_chain_->n_parts_;
          ++(act_part_chain_->n_parts_);
          act_part_ = act_part_chain_->parts_ + pos;
        }
        
        // finally allocate space and update free space and buffer
        act_part_->allocate(mpool_, last_alloc);
        act_buffer_       = act_part_->head_;
        act_free_bytes_   = act_part_->n_allocated_;
      }
      
      inline void
      write(std::function<size_t(char *)> fun)
      {
        using namespace google::protobuf::io;
        
        // have enough space, do the write
        if( act_free_bytes_ >= (LEN+2) )
        {
          auto * prev_buffer = act_buffer_;
          act_buffer_ = CodedOutputStream::WriteTagToArray(TAG, act_buffer_);
          // save the payload position so we can write it later
          auto * payload_pos = act_buffer_;
          ++act_buffer_;
          
          // calling the supplied function
          size_t used = fun((char *)act_buffer_);
          uint8_t u8_used = (uint8_t)(used>LEN?LEN:used);
          act_buffer_ += u8_used;
          // set payload as well
          *payload_pos = u8_used;
          act_free_bytes_ -= (2+u8_used);
          act_part_->n_used_ += (2+u8_used);
          return;
        }
        
        allocate_more_write_area();
        write(fun);
      }
    };
    
    template <uint32_t TAG, interface::pb::Kind KIND>
    class buffer_writer : public value_type_writer
    {
    protected:
      part_chain *  act_part_chain_;
      size_t        act_item_;
      
      inline buffer_writer(size_t estimated_item_count)
      : value_type_writer(KIND,
                          (128*estimated_item_count),
                          estimated_item_count),
        act_part_chain_{&root_}
      {
        using namespace google::protobuf::io;
        
        size_t parts_to_allocate = (estimated_item_count)+1;
        root_.allocate(mpool_,parts_to_allocate);
        root_.next_ = &nulls_;
        // type                            // 1
        // XValue { Tag, Size, Data } *    // (2,3)*estimated
        // IsNull { Tag, PayLoad, Data* }  // 4, 5->
        root_.parts_[0].allocate(mpool_, 16);
        
        // fill first buffer
        {
          uint8_t * spos = root_.parts_[0].data_;
          spos = CodedOutputStream::WriteTagToArray(1<<3, spos);
          spos = CodedOutputStream::WriteVarint32ToArray(KIND, spos);
          root_.parts_[0].n_used_ = spos-root_.parts_[0].head_;
          ++(root_.n_parts_);
        }
      }
      
      inline void
      write(size_t desired_size,
            std::function<size_t(char *,
                                 size_t)> fun)
      {
        using namespace google::protobuf::io;
        
        if( act_part_chain_->n_parts_ < act_part_chain_->n_allocated_ )
        {
          size_t pos = act_part_chain_->n_parts_;
          part * p = act_part_chain_->parts_ + pos;
          size_t to_allocate = 16+desired_size;
          // allocate space and pass it to the caller
          p->allocate(mpool_, to_allocate);
          p->data_ = p->head_+16;
          // calling the supplied function
          size_t used = fun((char *)p->data_, desired_size);
          size_t to_reuse = desired_size-used;
          mpool_.reuse<char>(to_reuse);
          ++(act_part_chain_->n_parts_);
          // update tag, payload and data start pointer and size
          p->n_allocated_ -= to_reuse;
          // write payload
          auto * payload_end = CodedOutputStream::WriteVarint32ToArray(used, p->head_);
          size_t payload_len = payload_end-p->head_;
          auto * data_start = p->data_ - payload_len;
          // copy payload to the right place
          ::memcpy(data_start, p->head_, payload_len);
          --data_start;
          // write tag too
          CodedOutputStream::WriteTagToArray(TAG, data_start);
          // update members
          p->data_ = data_start;
          p->n_used_ = used+payload_len+1;
          return;
        }
        
        // we have eaten up all space in the current part_chain, will need a new part_chain
        auto * new_chain = mpool_.allocate<part_chain>(1);
        new_chain->allocate(mpool_,act_part_chain_->n_allocated_);
        
        auto old_chain   = act_part_chain_;
        act_part_chain_  = new_chain;
        
        // chain this in
        act_part_chain_->next_  = old_chain->next_;
        old_chain->next_        = act_part_chain_;

        // need to retry this
        write(desired_size, fun);
      }
    };
    
    class string_writer : public buffer_writer<((2<<3)+2),interface::pb::Kind::STRING>
    {
      typedef buffer_writer<((2<<3)+2), interface::pb::Kind::STRING> parent_t;
      friend class value_type_writer;
      inline string_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_string(size_t desired_size,
                   std::function<size_t(char *,
                                        size_t)> fun)
      {
        write(desired_size, fun);
      }
    };
    
    class date_writer : public fixlen_writer<((2<<3)+2),interface::pb::Kind::DATE,8>
    {
      typedef fixlen_writer<((2<<3)+2),interface::pb::Kind::DATE,8> parent_t;
      friend class value_type_writer;
      inline date_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_fixlen(std::function<size_t(char *)> fun)
      {
        write(fun);
      }
    };
    
    class time_writer : public fixlen_writer<((2<<3)+2),interface::pb::Kind::TIME,6>
    {
      typedef fixlen_writer<((2<<3)+2),interface::pb::Kind::TIME,6> parent_t;
      friend class value_type_writer;
      inline time_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_fixlen(std::function<size_t(char *)> fun)
      {
        write(fun);
      }
    };
    
    class int32_writer : public packed_writer<int32_t,((3<<3)+2),interface::pb::Kind::INT32>
    {
      typedef packed_writer<int32_t,((3<<3)+2),interface::pb::Kind::INT32> parent_t;
      friend class value_type_writer;
      inline int32_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_int32(int32_t v)
      {
        uint32_t vv = v;
        write32(vv);
      }
    };
    
    class int64_writer : public packed_writer<int64_t,((4<<3)+2),interface::pb::Kind::INT64>
    {
      typedef packed_writer<int64_t,((4<<3)+2),interface::pb::Kind::INT64> parent_t;
      friend class value_type_writer;
      inline int64_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void write_int64(int64_t v)
      {
        uint64_t vv = v;
        write64(vv);
      }
    };
    
    class uint32_writer : public packed_writer<uint32_t,((5<<3)+2),interface::pb::Kind::UINT32>
    {
      typedef packed_writer<uint32_t,((5<<3)+2),interface::pb::Kind::UINT32> parent_t;
      friend class value_type_writer;
      inline uint32_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void write_uint32(uint32_t v)
      {
        write32(v);
      }
    };
    
    class uint64_writer : public packed_writer<uint64_t,((6<<3)+2),interface::pb::Kind::UINT64>
    {
      typedef packed_writer<uint64_t,((6<<3)+2),interface::pb::Kind::UINT64> parent_t;
      friend class value_type_writer;
      inline uint64_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_uint64(uint64_t v)
      {
        write64(v);
      }
    };
    
    class double_writer : public packed_writer<double,((7<<3)+2),interface::pb::Kind::DOUBLE>
    {
      typedef packed_writer<double,((7<<3)+2),interface::pb::Kind::DOUBLE> parent_t;
      friend class value_type_writer;
      inline double_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_double(double v)
      {
        write(v);
      }
    };
    
    class float_writer : public packed_writer<float,((8<<3)+2),interface::pb::Kind::FLOAT>
    {
      typedef packed_writer<float,((8<<3)+2),interface::pb::Kind::FLOAT> parent_t;
      friend class value_type_writer;
      inline float_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_float(float v)
      {
        write(v);
      }
    };
    
    class bool_writer : public packed_writer<bool,((9<<3)+2),interface::pb::Kind::BOOL>
    {
      typedef packed_writer<bool,((9<<3)+2),interface::pb::Kind::BOOL> parent_t;
      friend class value_type_writer;
      inline bool_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_bool(bool v)
      {
        write32((v?1:0));
      }
    };
    
    class bytes_writer : public buffer_writer<((10<<3)+2),interface::pb::Kind::BYTES>
    {
      typedef buffer_writer<((10<<3)+2),interface::pb::Kind::BYTES> parent_t;
      friend class value_type_writer;
      inline bytes_writer(size_t estimated_item_count) : parent_t(estimated_item_count) {}
    public:
      inline void
      write_bytes(size_t desired_size,
                  std::function<size_t(char *,
                                       size_t)> fun)
      {
        write(desired_size, fun);
      }
    };
  }
}}
