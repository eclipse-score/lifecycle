/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/mw/launch_manager/configuration/process_command.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using score::mw::lifecycle::internal::configuration::ProcessCommand;

class ProcessCommandTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");
    }
};

TEST_F(ProcessCommandTest, defaultConstructedIsEmpty)
{
    RecordProperty("Description", "Verify that a default constructed object has an empty path and no arguments");

    const ProcessCommand command;

    EXPECT_TRUE(command.executablePath().empty());
    EXPECT_TRUE(command.arguments().empty());
    ASSERT_NE(command.argv(), nullptr);
    EXPECT_STREQ(command.argv()[0], "");
    EXPECT_EQ(command.argv()[1], nullptr);
}

TEST_F(ProcessCommandTest, accessorsReturnConstructorValues)
{
    RecordProperty("Description", "Verify that the accessors return the executable path and arguments as given");

    const ProcessCommand command{"/bin/app", {"-a", "--long", "value"}};

    EXPECT_EQ(command.executablePath(), "/bin/app");
    EXPECT_EQ(command.arguments(), (std::vector<std::string>{"-a", "--long", "value"}));
}

TEST_F(ProcessCommandTest, argvStartsWithExecutablePathFollowedByArgumentsInOrder)
{
    RecordProperty(
        "Description",
        "Verify that argv() starts with the executable path, followed by the arguments in order and a "
        "null terminator");

    const ProcessCommand command{"/bin/app", {"-a", "--long", "value"}};

    char* const* argv = command.argv();
    EXPECT_STREQ(argv[0], "/bin/app");
    EXPECT_STREQ(argv[1], "-a");
    EXPECT_STREQ(argv[2], "--long");
    EXPECT_STREQ(argv[3], "value");
    EXPECT_EQ(argv[4], nullptr);
}

TEST_F(ProcessCommandTest, noArgumentsContainsOnlyExecutablePath)
{
    RecordProperty("Description", "Verify that argv() contains only the executable path if no arguments are given");

    const ProcessCommand command{"/bin/app", {}};

    EXPECT_STREQ(command.argv()[0], "/bin/app");
    EXPECT_EQ(command.argv()[1], nullptr);
}

TEST_F(ProcessCommandTest, manyArgumentsAreNotTruncated)
{
    RecordProperty("Description", "Verify that there is no fixed upper limit on the number of arguments");

    std::vector<std::string> args;
    for (int i = 0; i < 100; ++i)
    {
        args.push_back("arg" + std::to_string(i));
    }

    const ProcessCommand command{"/bin/app", args};

    EXPECT_EQ(command.arguments().size(), 100U);
    EXPECT_STREQ(command.argv()[100], "arg99");
    EXPECT_EQ(command.argv()[101], nullptr);
}

TEST_F(ProcessCommandTest, pointersAreValidAfterMoveConstruction)
{
    RecordProperty("Description", "Verify that argv() points to the moved-to storage after move construction");

    // Short strings are subject to the small string optimization, so their storage moves with the object
    ProcessCommand source{"/a", {"b"}};

    const ProcessCommand target{std::move(source)};

    EXPECT_EQ(target.argv()[0], target.executablePath().c_str());
    EXPECT_EQ(target.argv()[1], target.arguments()[0].c_str());
    EXPECT_STREQ(target.argv()[0], "/a");
    EXPECT_STREQ(target.argv()[1], "b");
    EXPECT_EQ(target.argv()[2], nullptr);
}

TEST_F(ProcessCommandTest, pointersAreValidAfterMoveAssignment)
{
    RecordProperty("Description", "Verify that argv() points to the moved-to storage after move assignment");

    ProcessCommand source{"/a", {"b"}};

    ProcessCommand target{"/other", {}};
    target = std::move(source);

    EXPECT_EQ(target.argv()[0], target.executablePath().c_str());
    EXPECT_EQ(target.argv()[1], target.arguments()[0].c_str());
    EXPECT_STREQ(target.argv()[0], "/a");
    EXPECT_STREQ(target.argv()[1], "b");
    EXPECT_EQ(target.argv()[2], nullptr);
}
