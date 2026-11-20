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
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

/// @file  verification_process.cpp
/// @brief Test process that touches signal files selected via command line
///        arguments. Pass "fallback_run_target" to touch the fallback file and/or
///        "test_end" to touch the test_end file. Optionally pass "delay_ms=<n>"
///        to wait <n> milliseconds before doing so.

int main(int argc, char** argv)
{
    bool touch_fallback = false;
    bool touch_test_end = false;
    constexpr std::string_view delay_prefix{"delay_ms="};
    std::chrono::milliseconds delay_ms{0};

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};
        if (arg == "fallback_run_target")
        {
            touch_fallback = true;
        }
        else if (arg == "test_end")
        {
            std::cout << "Touching test_end file." << std::endl;
            touch_test_end = true;
        }
        else if (arg.rfind(delay_prefix, 0) == 0)
        {
            const std::string value{arg.substr(delay_prefix.size())};
            char* end = nullptr;
            const long delay = std::strtol(value.c_str(), &end, 10);
            if (value.empty() || *end != '\0' || delay < 0)
            {
                std::cerr << "Invalid delay '" << value << "'" << std::endl;
                return EXIT_FAILURE;
            }
            delay_ms = std::chrono::milliseconds{delay};
        }
        else
        {
            std::cerr << "Unknown argument '" << arg << "', expected 'fallback_run_target', 'test_end' and/or '"
                      << delay_prefix << "<ms>'" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::this_thread::sleep_for(delay_ms);

    if (touch_fallback && !touch_file(fallback_file))
    {
        std::cout << "Failed to write fallback file!" << std::endl;
        return EXIT_FAILURE;
    }

    if (touch_test_end && kill(getppid(), SIGTERM) != 0)
    {
        std::cout << "Failed to signal test end!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
