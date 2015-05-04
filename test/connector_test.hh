#pragma once

#include <gtest/gtest.h>
#include <memory>

namespace virtdb { namespace connector {

  // forward declarations
  class client_context;
}}

namespace virtdb { namespace test {
  
  class ConnectorCommon : public ::testing::Test
  {
  protected:
    std::shared_ptr<connector::client_context> cctx_;
    virtual void SetUp();
  };
  
  class ConnEndpointTest : public ConnectorCommon { };
  class ConnConfigTest : public ConnectorCommon { };
  class ConnServerBaseTest : public ConnectorCommon { };

  class ConnCertStoreTest : public ConnectorCommon { };
  class ConnSrcsysCredTest : public ConnectorCommon { };
  
  class ConnIpDiscoveryTest : public ConnectorCommon { };
  class ConnLogRecordTest : public ConnectorCommon { };
  
  class ConnQueryTest : public ConnectorCommon { };
  class ConnColumnTest : public ConnectorCommon { };
  class ConnMetaDataTest : public ConnectorCommon { };
  
  class ConnPubSubTest : public ConnectorCommon { };
  class ConnPushPullTest : public ConnectorCommon { };
  class ConnReqRepTest : public ConnectorCommon { };
  
}}

