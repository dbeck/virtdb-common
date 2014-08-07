#include "logger_test.hh"

using namespace virtdb::test;
using namespace virtdb::interface;
using namespace virtdb::logger;

TEST_F(LoggerTest, LogInfo)
{
  LOG_INFO("info message");
}

TEST_F(LoggerTest, LogError)
{
  LOG_ERROR("error message");
}

TEST_F(LoggerTest, LogTrace)
{
  LOG_TRACE("trace message");
}

TEST_F(LoggerTest, LogScoped)
{
  LOG_SCOPED("scoped message");
}
