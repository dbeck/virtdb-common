#include "column.hh"
#include <util/exception.hh>

namespace virtdb { namespace datasrc {

  column::column(size_t max_rows)
  : max_rows_{max_rows},
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
  
  void
  column::set_seqno(size_t seqno)
  {
    auto & c = get_pb_column();
    c.set_seqno(seqno);
  }
  
  size_t
  column::max_rows() const
  {
    return max_rows_;
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
  column::compress()
  {
    // TODO
  }
  
  interface::pb::Column &
  column::get_pb_column()
  {
    return pb_column_;
  }
  
  void
  column::dispose(sptr p)
  {
    if( on_dispose_ )
      on_dispose_(std::move(p));
  }
  
  
  /////////////////
  
  fixed_width_column::fixed_width_column(size_t max_rows,
                                         size_t max_size)
  : parent_type{max_rows},
    data_{new char[max_rows*max_size]},
    actual_sizes_{max_rows, 0},
    max_size_{max_size}
  {
    if( !max_size )
    {
      THROW_("max_size parameter is zero");
    }
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

}}
