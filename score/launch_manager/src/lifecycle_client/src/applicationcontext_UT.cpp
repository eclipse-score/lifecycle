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

#include "score/mw/lifecycle/applicationcontext.h"
#include <gtest/gtest.h>
#include <cstdint>
#include <iterator>

namespace score::mw::lifecycle
{

class ApplicationContextTest : public ::testing::Test
{
  public:
    static constexpr const char* MISSING_FLAG = "--missing";
    static constexpr const char* EMPTY_STRING = "";
};

TEST_F(ApplicationContextTest, GivenProgramNameOnly_WhenGetArgumentsCalled_ThenReturnsSingleElementVector)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given argc/argv containing only the program name. "
        "When get_arguments() is called. "
        "Then the returned vector contains exactly the program name.");

    // Given
    const char* argv[] = {"test_app"};
    const ApplicationContext context(1, argv);

    // When
    const auto& args = context.get_arguments();

    // Then
    ASSERT_EQ(args.size(), 1U);
    EXPECT_EQ(args[0], argv[0]);
}

TEST_F(ApplicationContextTest, GivenMultipleArguments_WhenGetArgumentsCalled_ThenReturnsAllArgumentsInOrder)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given argc/argv containing several arguments. "
        "When get_arguments() is called. "
        "Then the returned vector contains all arguments, including the program name, in their original order. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "value1", "--flag2", "value2"};
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto& args = context.get_arguments();

    // Then
    ASSERT_EQ(args.size(), 5U);
    for (auto i = 0U; i < args.size(); ++i)
    {
        EXPECT_EQ(args[i], argv[i]);
    }
}

TEST_F(ApplicationContextTest, GivenFlagFollowedByValue_WhenGetArgumentCalled_ThenReturnsValue)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a flag present in argv, immediately followed by a value. "
        "When get_argument() is called with that flag. "
        "Then the following argument is returned. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "value1", "--flag2", "value2"};
    const char* FLAG2_STR = argv[3];
    const char* VALUE2_STR = argv[4];
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument(FLAG2_STR);

    // Then
    EXPECT_EQ(result, VALUE2_STR);
}

TEST_F(ApplicationContextTest, GivenFlagNotPresent_WhenGetArgumentCalled_ThenReturnsEmptyString)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a flag that is not present in argv. "
        "When get_argument() is called with that flag. "
        "Then an empty string is returned. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "value1"};
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument(ApplicationContextTest::MISSING_FLAG);

    // Then
    EXPECT_EQ(result, EMPTY_STRING);
}

TEST_F(ApplicationContextTest, GivenEmptyArgumentList_WhenGetArgumentCalled_ThenReturnsEmptyString)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given an ApplicationContext constructed with argc 0, so get_arguments() is empty. "
        "When get_argument() is called with any flag. "
        "Then an empty string is returned");

    // Given
    const char* argv[] = {"test_app"};
    const std::int32_t ZERO_SIZE = 0;
    const ApplicationContext context(ZERO_SIZE, argv);
    ASSERT_TRUE(context.get_arguments().empty());

    // When
    const auto result = context.get_argument(ApplicationContextTest::MISSING_FLAG);

    // Then
    EXPECT_EQ(result, ApplicationContextTest::EMPTY_STRING);
}

TEST_F(ApplicationContextTest, GivenFlagIsLastArgument_WhenGetArgumentCalled_ThenReturnsEmptyString)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a flag is present but it's the last argument (no following value). "
        "When get_argument() is called with that flag. "
        "Then an empty string is returned, since there is no value to return. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "value1", "--flag2"};
    const char* FLAG2_STR = argv[3];
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument(FLAG2_STR);

    // Then
    EXPECT_EQ(result, ApplicationContextTest::EMPTY_STRING);
}

TEST_F(ApplicationContextTest, GivenOnlyProgramName_WhenGetArgumentCalled_ThenReturnsEmptyString)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given argv containing only the program name. "
        "When get_argument() is called with any flag. "
        "Then an empty string is returned. ");

    // Given
    const char* argv[] = {"test_app"};
    const char* ANY_FLAG = "--flag1";
    const ApplicationContext context(1, argv);

    // When
    const auto result = context.get_argument(ANY_FLAG);

    // Then
    EXPECT_EQ(result, "");
}

TEST_F(ApplicationContextTest, GivenDuplicateFlags_WhenGetArgumentCalled_ThenReturnsValueFollowingFirstMatch)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given the same flag appearing more than once in argv. "
        "When get_argument() is called with that flag. "
        "Then the value following the first occurrence is returned. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "first", "--flag1", "second"};
    const char* FLAG1_STR = argv[1];
    const char* VALUE1_STR = argv[2];
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument(FLAG1_STR);

    // Then
    EXPECT_EQ(result, VALUE1_STR);
}

TEST_F(ApplicationContextTest, GivenFlagSubstringOfAnotherArgument_WhenGetArgumentCalled_ThenReturnsEmptyString)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given an argument that merely contains the flag as a substring (not an exact match). "
        "When get_argument() is called with that flag. "
        "Then an empty string is returned, since matching requires an exact equality. ");

    // Given
    const char* argv[] = {"test_app", "--flag1extra", "value1"};
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument("--flag1");

    // Then
    EXPECT_EQ(result, ApplicationContextTest::EMPTY_STRING);
}

TEST_F(ApplicationContextTest, GivenValueLooksLikeAFlag_WhenGetArgumentCalled_ThenReturnsTheLiteralFollowingValue)
{
    RecordProperty("DerivationTechnique", "explorative-testing");
    RecordProperty(
        "Description",
        "Given a flag whose following value happens to look like another flag. "
        "When get_argument() is called with the first flag. "
        "Then the literal following argument is returned, regardless of its shape. ");

    // Given
    const char* argv[] = {"test_app", "--flag1", "--looks-like-a-flag"};
    const char* FLAG1_STR = argv[1];
    const char* VALUE1_STR = argv[2];
    const ApplicationContext context(std::size(argv), argv);

    // When
    const auto result = context.get_argument(FLAG1_STR);

    // Then
    EXPECT_EQ(result, VALUE1_STR);
}

}  // namespace score::mw::lifecycle
