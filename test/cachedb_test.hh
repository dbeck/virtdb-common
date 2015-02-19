#pragma once

#include <gtest/gtest.h>

namespace virtdb { namespace test {
  
  class CachedbStoreTest      : public ::testing::Test { };
  class CachedbDbIdTest       : public ::testing::Test { };
  class CachedbHashUtilTest   : public ::testing::Test { };
  
  // new classes
  class CachedbColumnDataTest : public ::testing::Test { };
  class CachedbDBTest         : public ::testing::Test { };

}}
