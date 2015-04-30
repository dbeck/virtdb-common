#pragma once

#include <memory>
#include <string>

namespace virtdb { namespace test {
  
  class CfgSvcMock
  {
  private:
    struct impl;
    std::unique_ptr<impl> impl_;
    
    CfgSvcMock() = delete;
    CfgSvcMock & operator=(const CfgSvcMock &) = delete;
    CfgSvcMock(const CfgSvcMock &) = delete;
    
  public:
    CfgSvcMock(const std::string & ep);
    virtual ~CfgSvcMock();
  };
  
}}
