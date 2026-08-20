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

#include "tests/utils/test_helper/test_helper.hpp"
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

/// @file  file_deleting_process.cpp
/// @brief Test process that removes the file its component uses as a ready
///        condition. The path is passed as the first command line argument and
///        the file is expected to exist when the process starts.
///        The file is only removed after a delay, so a launch manager that does
///        not wait for it to disappear lets dependent components start too early.

namespace
{

/// @brief Time between process start and the removal of the ready condition file.
constexpr std::chrono::milliseconds kRemovalDelay{300};

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Expected the path of the ready condition file as the only argument" << std::endl;
        return EXIT_FAILURE;
    }

    std::this_thread::sleep_for(kRemovalDelay);

    std::error_code error{};
    if (!std::filesystem::remove(argv[1], error))
    {
        std::cerr << "Could not remove ready condition file " << argv[1] << ": "
                  << (error ? error.message() : "the file did not exist") << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Removed ready condition file " << argv[1] << std::endl;

    // Keep providing the "service" until the launch manager terminates us.
    pause();
    return EXIT_SUCCESS;
}
