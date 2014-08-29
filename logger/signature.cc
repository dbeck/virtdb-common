#include "signature.hh"

namespace virtdb { namespace logger {

  void signature::part::add_signature(const char * val,
                                      signature * sig)
  {
    auto pb_header = sig->get_pb_header();
    auto log_part = pb_header->add_parts();
    log_part->set_isvariable(false);
    log_part->set_hasdata(false);
    log_part->set_partsymbol(symbol_store::get_symbol_id(val));
  }
  
  signature::part::part(const end_msg &, part * parent)
  : signature_(parent->signature_)
  {
  }
  
  signature::part::part(const end_msg &, signature * sig)
  : signature_(sig)
  {
  }
  
  signature::signature()
  : header_sptr_(new interface::pb::LogHeader)
  {
  }
  
  signature::pb_header_sptr
  signature::get_pb_header() const
  {
    return header_sptr_;
  }
  
}}
