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
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "tests/utils/test_helper/test_helper.hpp"

/// @brief Fails the test explicitly.
///        Use this in run targets that shall not be reached during a test, to
///        explicitly fail a test if that run target is reached.
///
/// @details This creates the test_failed file, which the test script asserts
///          does not exist, and ends the test by sending SIGTERM to its parent
///          (the Launch Manager), so the test fails.
///

int main()
{
    std::cout << "Failing the test!" << std::endl;

    if (!touch_file(test_failed_file))
    {
        std::cout << "Failed to write test_failed file!" << std::endl;
        return EXIT_FAILURE;
    }

    if (kill(getppid(), SIGTERM) != 0)
    {
        std::cout << "Failed to signal test end!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
