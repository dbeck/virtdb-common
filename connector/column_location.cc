#include "column_location.hh"

namespace virtdb { namespace connector {
  
  bool
  column_location::operator<(const column_location &rhs)
  {
    if( query_id_ < rhs.query_id_ ) return true;
    else if( query_id_ > rhs.query_id_ ) return false;
    
    if( schema_ < rhs.schema_ ) return true;
    else if( schema_ > rhs.schema_ ) return false;
    
    if( table_ < rhs.table_ ) return true;
    else if( table_ > rhs.table_ ) return false;
    
    if( column_ < rhs.column_ ) return true;
    else return false;
  }
  
  bool
  column_location::is_wildcard() const
  {
    return (query_id_.empty() ||
            schema_.empty() ||
            table_.empty() ||
            column_.empty() );
  }

  bool
  column_location::is_wildcard_table() const
  {
    return (query_id_.empty() ||
            schema_.empty() ||
            table_.empty());
  }
  
  void
  column_location::init_table(const interface::pb::Query & q)
  {
    if( q.has_queryid() )
      query_id_ = q.queryid();
    
    if( q.has_schema() )
      schema_ = q.schema();
    
    if( q.has_table() )
      table_ = q.table();
  }
  
}}
