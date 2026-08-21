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
#include <gtest/gtest.h>
#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <thread>

enum class Operation : std::uint8_t
{
    Create = 0,
    Delete
};
std::string_view g_file_path;
std::chrono::milliseconds g_modify_delay;
Operation g_operation;

TEST(ModifyFile, ModifyFile)
{
    std::this_thread::sleep_for(g_modify_delay);
    switch (g_operation)
    {
        case (Operation::Create):
            ASSERT_TRUE(touch_file(g_file_path));
            break;
        case (Operation::Delete):
            std::error_code error{};
            ASSERT_TRUE(std::filesystem::remove(g_file_path, error))
                << "Could not remove file " << g_file_path << ": "
                << (error ? error.message() : "the file did not exist");
            break;
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "USAGE:" << argv[0] << "(file path) (milliseconds to wait before doing the operation)"
                  << std::endl;
        return EXIT_FAILURE;
    }
    const std::string_view prog_name{argv[0]};

    if (prog_name == "file_creator")
    {
        g_operation = Operation::Create;
    }
    else if (prog_name == "file_deletor")
    {
        g_operation = Operation::Delete;
    }

    g_file_path = std::string_view{argv[1]};
    g_modify_delay = std::chrono::milliseconds{std::stoi(argv[2])};

    std::string xml_name{"reporting_process_"};
    xml_name.append(prog_name);
    return TestRunner(xml_name).RunTests();
}
