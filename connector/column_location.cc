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
  
}}
