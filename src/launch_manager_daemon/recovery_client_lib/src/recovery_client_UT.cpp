/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#include <gtest/gtest.h>

#include <score/lcm/recovery_client.hpp>

namespace score {
namespace lcm {

class RecoveryClientTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

TEST_F(RecoveryClientTest, SendSingleRequest)
{
    RecordProperty("Description",
                   "RecoveryClient can send single request successfully.");

    RecoveryClient client;
    const bool result = client.sendRecoveryRequest(IdentifierHash("pg_a"));
    EXPECT_TRUE(result);
    EXPECT_FALSE(client.hasOverflow());
}

TEST_F(RecoveryClientTest, GetNextRequest)
{
    RecordProperty("Description",
                   "RecoveryClient can send and retrieve single request successfully.");

    RecoveryClient client;
    const IdentifierHash expected_pg("pg_b");
    client.sendRecoveryRequest(expected_pg);

    const auto req = client.getNextRequest();
    ASSERT_TRUE(req.has_value());
    EXPECT_EQ(req->process_group_identifier_, expected_pg);
}

TEST_F(RecoveryClientTest, GetNextRequestEmpty)
{
    RecordProperty("Description",
                   "RecoveryClient returns no request when buffer is empty.");

    RecoveryClient client;
    EXPECT_FALSE(client.getNextRequest().has_value());
}

TEST_F(RecoveryClientTest, RingBufferFull)
{
    RecordProperty("Description",
                   "RecoveryClient sets overflow flag if buffer is full.");

    RecoveryClient client;
    const IdentifierHash pg("pg_c");

    // Fill the ring buffer
    for (std::size_t i = 0U; i < RecoveryClient::kBufferCapacity; ++i)
    {
        EXPECT_TRUE(client.sendRecoveryRequest(pg));
    }

    // One more should fail and set overflow
    EXPECT_FALSE(client.sendRecoveryRequest(pg));
    EXPECT_TRUE(client.hasOverflow());
}

TEST_F(RecoveryClientTest, FIFOOrdering)
{
    RecordProperty("Description",
                   "RecoveryClient maintains the order of inserted requests");

    RecoveryClient client;
    const IdentifierHash pg_first("pg_first");
    const IdentifierHash pg_second("pg_second");
    const IdentifierHash pg_third("pg_third");

    client.sendRecoveryRequest(pg_first);
    client.sendRecoveryRequest(pg_second);
    client.sendRecoveryRequest(pg_third);

    const auto req1 = client.getNextRequest();
    ASSERT_TRUE(req1.has_value());
    EXPECT_EQ(req1->process_group_identifier_, pg_first);

    const auto req2 = client.getNextRequest();
    ASSERT_TRUE(req2.has_value());
    EXPECT_EQ(req2->process_group_identifier_, pg_second);

    const auto req3 = client.getNextRequest();
    ASSERT_TRUE(req3.has_value());
    EXPECT_EQ(req3->process_group_identifier_, pg_third);

    EXPECT_FALSE(client.getNextRequest().has_value());
}

} // namespace lcm
} // namespace score
