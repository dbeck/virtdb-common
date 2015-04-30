#pragma once

#include <gtest/gtest.h>

namespace virtdb { namespace test {
  
  class ConnectorCommon : public ::testing::Test
  {
  protected:
    virtual void SetUp();
    virtual void TearDown();    
    virtual ~ConnectorCommon();
  };
  
  class ConnEndpointClientTest : public ConnectorCommon { };
  class ConnServerBaseTest : public ConnectorCommon { };
  class ConnConfigClientTest : public ConnectorCommon { };

  
  class ConnColumnClientTest : public ConnectorCommon { };
  class ConnColumnServerTest : public ConnectorCommon { };

  class ConnConfigServerTest : public ConnectorCommon { };
  
  class ConnDbConfigClientTest : public ConnectorCommon { };
  class ConnDbConfigServerTest : public ConnectorCommon { };
  
  class ConnEndpointServerTest : public ConnectorCommon { };
  
  class ConnIpDiscoveryClientTest : public ConnectorCommon { };
  class ConnIpDiscoveryServerTest : public ConnectorCommon { };
  
  class ConnLogRecordClientTest : public ConnectorCommon { };
  class ConnLogRecordServerTest : public ConnectorCommon { };
  
  class ConnMetaDataClientTest : public ConnectorCommon { };
  class ConnMetaDataServerTest : public ConnectorCommon { };

  class ConnSubClientTest : public ConnectorCommon { };
  class ConnPubServerTest : public ConnectorCommon { };

  class ConnPushClientTest : public ConnectorCommon { };
  class ConnPullServerTest : public ConnectorCommon { };

  class ConnQueryClientTest : public ConnectorCommon { };
  class ConnQueryServerTest : public ConnectorCommon { };

  class ConnReqClientTest : public ConnectorCommon { };
  class ConnRepServerTest : public ConnectorCommon { };
  
}}

