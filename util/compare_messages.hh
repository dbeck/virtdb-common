#pragma once

#include <svc_config.pb.h>
#include <diag.pb.h>

namespace virtdb { namespace util {
  
  struct compare_endpoint_data
  {
    bool operator()(const interface::pb::EndpointData & lhs,
                    const interface::pb::EndpointData & rhs) const
    {
      if( lhs.name() < rhs.name() )
        return true;
      else if (lhs.name() > rhs.name() )
        return false;
      
      if( lhs.svctype() < rhs.svctype() )
        return true;
      else
        return false;
    }
  };
  
  struct compare_process_info
  {
    bool operator()(const interface::pb::ProcessInfo & lhs,
                    const interface::pb::ProcessInfo & rhs) const
    {
      if( lhs.startdate() < rhs.startdate() )
        return true;
      else if( lhs.startdate() > rhs.startdate() )
        return false;
      
      if( lhs.starttime() < rhs.starttime() )
        return true;
      else if( lhs.starttime() > rhs.starttime() )
        return false;
      
      if( lhs.pid() < rhs.pid() )
        return true;
      else if( lhs.pid() > rhs.pid() )
        return false;
      
      if( lhs.random() < rhs.random() )
        return true;
      else if( lhs.random() > rhs.random() )
        return false;
      else
        return false;
    }
  };

}}
