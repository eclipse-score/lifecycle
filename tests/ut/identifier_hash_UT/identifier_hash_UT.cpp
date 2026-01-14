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
#include <gtest/gtest.h>
#include <score/lcm/identifier_hash.hpp>
#include <memory>
#include <sstream>

using namespace testing;
using std::stringstream;

using score::lcm::IdentifierHash;

TEST(IdentifierHashTest, IdentifierHash_with_string_view_created)
{
    std::string_view idStrView = "ProcessGroup1/Startup";
    IdentifierHash identifierHash(idStrView);
    stringstream strStream;
    strStream << identifierHash;
    ASSERT_EQ(strStream.str(), idStrView);
}

TEST(IdentifierHashTest, IdentifierHash_with_string_created)
{
    std::string idStr = "ProcessGroup1/Startup";
    IdentifierHash identifierHash(idStr);
    stringstream strStream;
    strStream << identifierHash;
    ASSERT_EQ(strStream.str(), idStr);
}

TEST(IdentifierHashTest, IdentifierHash_default_created)
{
    IdentifierHash identifierHash;
    stringstream strStream;
    strStream << identifierHash;
    ASSERT_EQ(strStream.str(), "");
}

TEST(IdentifierHashTest, IdentifierHash_invalid_hash_no_string_representation)
{
    std::string idStr = "MainFG";
    IdentifierHash identifierHash(idStr);

    // Clear registry to simulate missing entry
    IdentifierHash::get_registry().clear();

    stringstream strStream;
    strStream << identifierHash;
    ASSERT_TRUE(strStream.str().find("Unknown IdentifierHash") != std::string::npos);
    ASSERT_TRUE(strStream.str().find(std::to_string(identifierHash.data())) != std::string::npos);
}

TEST(IdentifierHashTest, IdentifierHash_no_dangling_pointer_after_source_string_dies)
{
    std::unique_ptr<IdentifierHash> hash_ptr;
    std::string_view idStrView = "this string will be destroyed";

    {
        std::string tmpIdStr = std::string(idStrView);
        hash_ptr = std::make_unique<IdentifierHash>(tmpIdStr);  // Registry stores an owned copy
        // Do not read the registry yet; ensure we read after the source string is destroyed.
    }

    stringstream strStream;
    strStream << *(hash_ptr.get());

    ASSERT_EQ(strStream.str(), idStrView);
}
