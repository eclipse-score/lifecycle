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

#include <concurrency/concurrency_error_domain.hpp>

#include <gtest/gtest.h>

using namespace score::lcm::internal;

class ConcurrencyErrorDomainMessageForTest : public ::testing::TestWithParam<score::result::ErrorCode>
{
};

TEST_P(ConcurrencyErrorDomainMessageForTest, CoversAllBranches)
{
    const score::result::ErrorDomain& domain = g_ConcurrencyErrorDomain;
    static_cast<void>(domain.MessageFor(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(AllCodes,
                         ConcurrencyErrorDomainMessageForTest,
                         ::testing::Values(static_cast<score::result::ErrorCode>(ConcurrencyErrc::kOsError),
                                           static_cast<score::result::ErrorCode>(ConcurrencyErrc::kOverflow),
                                           static_cast<score::result::ErrorCode>(ConcurrencyErrc::kStopped),
                                           static_cast<score::result::ErrorCode>(ConcurrencyErrc::kTimeout),
                                           99));  // default branch

TEST(ConcurrencyErrorDomainTest, MakeError)
{
    static_cast<void>(MakeError(ConcurrencyErrc::kOsError));
    static_cast<void>(MakeError(ConcurrencyErrc::kOsError, "detail"));
}
