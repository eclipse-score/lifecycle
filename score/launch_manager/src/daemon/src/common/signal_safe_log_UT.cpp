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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "score/mw/launch_manager/common/signal_safe_log.hpp"

using namespace testing;
using score::mw::lifecycle::internal::signal_safe_log;
using score::mw::lifecycle::internal::signal_safe_log_errno;

class signal_safe_log_test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(signal_safe_log_test, signal_safe_log_test_simple)
{
    RecordProperty(
        "Description", "This test verifies that signal_safe_log successfully writes a simple message to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log("Hello world"));

    EXPECT_EQ(testing::internal::GetCapturedStderr(), "Hello world\n");
}

TEST_F(signal_safe_log_test, signal_safe_log_test_simple_with_errno)
{
    RecordProperty(
        "Description",
        "This test verifies that signal_safe_log successfully writes a simple message and errno to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log_errno(1, "Hello world"));

    EXPECT_THAT(testing::internal::GetCapturedStderr(), ContainsRegex("^Hello world \\(.*\\)"));
}

TEST_F(signal_safe_log_test, signal_safe_log_test_formatted)
{
    RecordProperty(
        "Description", "This test verifies that signal_safe_log successfully writes a formatted message to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(12, " + ", 76, " = ", 88));
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "12 + 76 = 88\n");
}

TEST_F(signal_safe_log_test, signal_safe_log_test_formatted_with_errno)
{
    RecordProperty(
        "Description",
        "This test verifies that signal_safe_log successfully writes a formatted message and errno to stderr.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log_errno(1, 12, " + ", 76, " = ", 88));

    EXPECT_THAT(testing::internal::GetCapturedStderr(), ContainsRegex("12 \\+ 76 = 88 \\(.*\\)"));
}

TEST_F(signal_safe_log_test, signal_safe_log_test_minimum_length)
{
    RecordProperty("Description", "This test verifies that signal_safe_log can output at least 1024 characters.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(std::string(1023, 'A')));

    const auto stderr = testing::internal::GetCapturedStderr();

#ifdef __QNXNTO__
    EXPECT_EQ(stderr->size(), 1024);
    EXPECT_EQ(stderr->back(), '\n');
#else
    EXPECT_EQ(stderr.size(), 1024);
    EXPECT_EQ(stderr.back(), '\n');
#endif
}

TEST_F(signal_safe_log_test, signal_safe_log_test_maximum_length)
{
    RecordProperty("Description", "This test verifies that signal_safe_log can output at most 1024 characters.");

    testing::internal::CaptureStderr();
    ASSERT_TRUE(signal_safe_log(std::string(2048, 'A')));

    const auto stderr = testing::internal::GetCapturedStderr();

#ifdef __QNXNTO__
    EXPECT_EQ(stderr->size(), 1024);
    EXPECT_EQ(stderr->back(), 'A');
#else
    EXPECT_EQ(stderr.size(), 1024);
    EXPECT_EQ(stderr.back(), 'A');
#endif
}

class signal_safe_buffer_test : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(signal_safe_buffer_test, GivenFullBuffer_WhenIntegerAdded_ThenSilentFailure)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a full buffer. "
        "When an integer is added to the buffer using append(). "
        "Then the integer is not added. "
        "And no error is reported.");

    // Given
    signal_safe_buffer<4U> buffer = {};
    buffer.append("abcd");

    // When
    buffer.append(12345);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "abcd");
}

TEST_F(signal_safe_buffer_test, GivenEmptyBuffer_WhenIntegerExactlyFillsBuffer_ThenIntegerIsAdded)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given an empty buffer. "
        "When an integer is added that exactly fills the buffer using append(). "
        "Then the whole integer is added, including the final byte.");

    // Given
    signal_safe_buffer<4U> buffer = {};

    // When
    buffer.append(1234);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "1234");
}

TEST_F(signal_safe_buffer_test, GivenBufferWithOneByteFree_WhenSingleDigitIntegerAdded_ThenIntegerIsAdded)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a buffer with exactly one byte free. "
        "When a single digit integer is added using append(). "
        "Then the integer is added into the last byte.");

    // Given
    signal_safe_buffer<4U> buffer = {};
    buffer.append("abc");

    // When
    buffer.append(1);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "abc1");
}

TEST_F(signal_safe_buffer_test, GivenBufferWithOneByteFree_WhenMultiDigitIntegerAdded_ThenSilentFailure)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given a buffer with exactly one byte free. "
        "When an integer needing more than one byte is added using append(). "
        "Then the integer is not added, not even partially. "
        "And no error is reported.");

    // Given
    signal_safe_buffer<4U> buffer = {};
    buffer.append("abc");

    // When
    buffer.append(42);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "abc");
}

TEST_F(signal_safe_buffer_test, GivenEmptyBuffer_WhenNegativeIntegerExactlyFillsBuffer_ThenIntegerIsAdded)
{
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty(
        "Description",
        "Given an empty buffer. "
        "When a negative integer, including its sign, exactly fills the buffer using append(). "
        "Then the whole integer is added.");

    // Given
    signal_safe_buffer<4U> buffer = {};

    // When
    buffer.append(-123);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "-123");
}

TEST_F(signal_safe_buffer_test, GivenBufferAfterFailedIntegerAppend_WhenSmallerIntegerAdded_ThenIntegerIsAdded)
{
    RecordProperty(
        "Description",
        "Given a buffer where appending a too large integer failed. "
        "When a smaller integer that fits is added using append(). "
        "Then the smaller integer is added directly after the existing content.");

    // Given
    signal_safe_buffer<4U> buffer = {};
    buffer.append("ab");
    buffer.append(12345);

    // When
    buffer.append(9);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "ab9");
}

TEST_F(signal_safe_buffer_test, GivenBufferWithIntegers_WhenIntegersAppendedSequentially_ThenOffsetsAreAccumulated)
{
    RecordProperty(
        "Description",
        "Given an empty buffer. "
        "When several integers and strings are appended in sequence using append(). "
        "Then each value is placed directly after the previous one, "
        "showing the pointer difference is converted to the correct unsigned offset.");

    // Given
    signal_safe_buffer<32U> buffer = {};

    // When
    buffer.append(1);
    buffer.append("+");
    buffer.append(22);
    buffer.append("=");
    buffer.append(-23);
    buffer.append(std::size_t{18446744073709551615ULL});

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "1+22=-2318446744073709551615");
}

TEST_F(signal_safe_buffer_test, GivenFlushedBuffer_WhenBufferReused_ThenOnlyNewContentIsWritten)
{
    RecordProperty(
        "Description",
        "Given a buffer that has been flushed. "
        "When new content is appended and flushed again. "
        "Then only the new content is written, so the indices were rewound correctly.");

    // Given
    signal_safe_buffer<16U> buffer = {};
    buffer.append("first");
    testing::internal::CaptureStderr();
    ASSERT_TRUE(buffer.flush());
    ASSERT_EQ(testing::internal::GetCapturedStderr(), "first");

    // When
    buffer.append(42);

    // Then
    testing::internal::CaptureStderr();
    EXPECT_TRUE(buffer.flush());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "42");
}
