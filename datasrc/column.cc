#ifdef RELEASE
#define LOG_TRACE_IS_ENABLED false
#define LOG_SCOPED_IS_ENABLED false
#endif //RELEASE

#include "column.hh"
#include <util/exception.hh>
#include <util/flex_alloc.hh>
#include <logger.hh>
#include <lz4/lz4.h>

namespace virtdb { namespace datasrc {

  column::column(size_t max_rows)
  : max_rows_{max_rows},
    n_rows_{0},
    nulls_(max_rows, false)
  {
    if( !max_rows )
    {
      THROW_("max_rows parameter is zero");
    }
  }
  
  void
  column::set_last()
  {
    auto & c = get_pb_column();
    c.set_endofdata(true);
  }
  
  bool
  column::is_last()
  {
    auto & c = get_pb_column();
    return (c.has_endofdata() && c.endofdata());
  }
  
  void
  column::seqno(size_t seqno)
  {
    auto & c = get_pb_column();
    c.set_seqno(seqno);
  }
  
  size_t
  column::seqno()
  {
    auto & c = get_pb_column();
    return c.seqno();
  }
  
  size_t
  column::max_rows() const
  {
    return max_rows_;
  }
  
  size_t
  column::n_rows() const
  {
    return n_rows_;
  }
  
  void
  column::n_rows(size_t n)
  {
    n_rows_ = n;
  }
  
  column::null_vector &
  column::nulls()
  {
    return nulls_;
  }
  
  void
  column::set_on_dispose(on_dispose d)
  {
    on_dispose_ = d;    
  }
  
  void
  column::prepare()
  {
    n_rows_ = 0;
    auto & c = get_pb_column();
    c.Clear();
  }
  
  void
  column::compress()
  {
    auto & c = get_pb_column();
    auto * dta = c.mutable_data();
    auto dtype = dta->type();
    int byte_size = dta->ByteSize();
    
    util::flex_alloc<char, 2048> uncompressed_buffer{byte_size};
    if( dta->SerializeToArray(uncompressed_buffer.get(), byte_size) )
    {
      int max_compressed_size = LZ4_compressBound(byte_size);
      util::flex_alloc<char, 2048> compressed_buffer{max_compressed_size};
      
      int lz4ret = LZ4_compress(uncompressed_buffer.get(),
                                compressed_buffer.get(),
                                byte_size);
      if( !lz4ret )
      {
        LOG_ERROR("LZ4 compression failed");
        return;
      }
      
#if 0
      LOG_TRACE("block compressed" <<
                V_(lz4ret) <<
                V_(byte_size) <<
                V_(c.queryid()) <<
                V_(c.name()) <<
                V_(c.seqno()));
#endif
      
      // clear original data, but set type
      dta->Clear();
      dta = c.mutable_data();
      dta->set_type(dtype);
     
      // set compression properties
      c.set_comptype(interface::pb::CompressionType::LZ4_COMPRESSION);
      c.set_compresseddata(compressed_buffer.get(), lz4ret);
      c.set_uncompressedsize(byte_size);
    }
  }
  
  interface::pb::Column &
  column::get_pb_column()
  {
    return pb_column_;
  }
  
  void
  column::dispose(sptr && p)
  {
    if( on_dispose_ && p )
      on_dispose_(std::move(p));
  }
    
  /////////////////
  
  fixed_width_column::fixed_width_column(size_t max_rows,
                                         size_t max_size)
  : parent_type{max_rows},
    data_{new char[max_rows*max_size]},
    max_size_{max_size},
    in_field_offset_{0}
  {
    if( !max_size )
    {
      THROW_("max_size parameter is zero");
    }
    actual_sizes_.reserve(max_rows);
    for( size_t i=0; i<max_rows; ++i )
      actual_sizes_.push_back(0);
  }

  void
  fixed_width_column::free_temp_data()
  {
    if( max_size() > 32 )
    {
      data_.reset();
    }
  }

  void
  fixed_width_column::prepare()
  {
    if( !data_ )
    {
      data_.reset(new char[max_rows()*max_size()]);
    }
    column::prepare();
  }
  
  fixed_width_column::size_vector &
  fixed_width_column::actual_sizes()
  {
    return actual_sizes_;
  }
  
  size_t
  fixed_width_column::max_size() const
  {
    return max_size_;
  }
  
  char *
  fixed_width_column::get_ptr()
  {
    return data_.get();
  }
  
  void
  fixed_width_column::in_field_offset(size_t o)
  {
    in_field_offset_ = o;
  }
  
  size_t
  fixed_width_column::in_field_offset() const
  {
    return in_field_offset_;
  }


}}
