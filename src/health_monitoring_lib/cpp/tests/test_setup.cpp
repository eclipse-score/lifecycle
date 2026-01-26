#include "score/mw/log/rust/stdout_logger_init.h"
#include <gtest/gtest.h>
#include <iostream>

class LogEnvironment : public ::testing::Environment
{
  public:
    void SetUp() override
    {
        using namespace score::mw::log::rust;

        StdoutLoggerBuilder builder;
        builder.Context("TEST").LogLevel(LogLevel::Verbose).SetAsDefaultLogger();
    }

    void TearDown() override
    {
        // Runs once after all tests
    }
};

struct RegisterLogEnvironment
{
    RegisterLogEnvironment()
    {
        ::testing::AddGlobalTestEnvironment(new LogEnvironment());
    }
};

static RegisterLogEnvironment s_register_log_environment;
