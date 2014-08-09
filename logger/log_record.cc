#include "log_record.hh"

namespace virtdb { namespace logger {
  
  // the last item in the list
  log_record::sender::sender(const end_msg &, log_record::sender * parent)
  : record_(parent->record_),
    parent_(parent),
    root_(parent->root_)
  {
    assert( root_ != nullptr );
    if( root_ && root_->pb_record_ )
    {
      auto sink = log_sink::get_sptr();
      if( sink )
        return sink->send_record(root_->pb_record_);
    }
  }
  
  log_record::sender::sender(const end_msg &, log_record * record)
  : record_(record),
    parent_(nullptr),
    root_(this)
  {
    // ignoring this one because it has no data
    // shouldn't happen at all
  }
  
  // iterating over the last item
  log_record::sender
  log_record::sender::operator<<(const end_msg & v)
  {
    return log_record::sender(v, this);
  }


  log_record::log_record(const char *             file,
                         uint32_t                 line,
                         const char *             func,
                         log_level                level,
                         bool                     enabled,
                         const signature::part &  part,
                         const char *             msg)
  :
    id_(header_store::get_new_id(this)),
    file_symbol_(symbol_store::get_symbol_id(file)),
    line_(line),
    func_symbol_(symbol_store::get_symbol_id(func)),
    level_(level),
    enabled_(enabled),
    msg_symbol_(symbol_store::get_symbol_id(msg))
  {
    // TODO : prepare the header to be included in the message
    header_.set_seqno(id_);
    header_.set_filenamesymbol(file_symbol_);
    header_.set_linenumber(line_);
    header_.set_functionnamesymbol(func_symbol_);
    header_.set_level(level_);
    header_.set_logstringsymbol(msg_symbol_);
    // TODO : fill: LogParts
    // header_.add_parts()
    // use parts here ...
    std::cout << msg<< "\n";
  }

  void
  log_record::on_return() const
  {
    // TODO : send an end-scope message
    std::cout << "msg symbol:" << msg_symbol_ << " RET \n";
  }
  
  bool
  log_record::enabled() const
  {
    return enabled_;
  }
  
  uint32_t
  log_record::id() const
  {
    return id_;
  }
  
  const interface::pb::LogHeader &
  log_record::get_pb_header() const
  {
    return header_;
  }
  
  
}}
