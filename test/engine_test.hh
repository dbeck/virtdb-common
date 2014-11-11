#pragma once

#include <gtest/gtest.h>
#include <datasrc.hh>

namespace virtdb { namespace test {
  
  class ChunkStoreTest : public ::testing::Test { };
  class ColumnChunkTest : public ::testing::Test { };
  class DataChunkTest : public ::testing::Test { };
  class ExpressionTest : public ::testing::Test { };
  class QueryTest : public ::testing::Test { };
  class ReceiverThreadTest : public ::testing::Test { };

}}
