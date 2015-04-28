#pragma once

namespace virtdb { namespace interface { namespace pb {
  class EndpointData;
  class ProcessInfo;
}}}

namespace virtdb { namespace util {
  
  struct compare_endpoint_data
  {
    bool operator()(const interface::pb::EndpointData & lhs,
                    const interface::pb::EndpointData & rhs) const;
  };
  
  struct compare_process_info
  {
    bool operator()(const interface::pb::ProcessInfo & lhs,
                    const interface::pb::ProcessInfo & rhs) const;
  };

}}
