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
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>
#include <cctype>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "tests/integration/sandbox_options/verify_sandbox.hpp"
#include "tests/utils/test_helper/test_helper.hpp"

namespace
{

using sandbox_options::ExpectedValues;

// Expected sandbox option values, supplied to this process on the command line.
//
// The launch manager applies the sandbox options from sandbox_options.json and passes the same
// values to the managed process via 'process_arguments', so the test verifies the applied state
// against the configured expectation without duplicating any literals here.
//
// Populated from argv in main() before the tests run.
ExpectedValues expected;

int parse_policy(const std::string& name)
{
    if (name == "SCHED_FIFO")
    {
        return SCHED_FIFO;
    }
    if (name == "SCHED_RR")
    {
        return SCHED_RR;
    }
    if (name == "SCHED_OTHER")
    {
        return SCHED_OTHER;
    }
    return -1;
}

// Parse a comma separated list of group ids, e.g. "123,321,456".
std::vector<gid_t> parse_groups(const std::string& csv)
{
    std::vector<gid_t> groups;
    std::stringstream stream(csv);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        if (!token.empty())
        {
            groups.push_back(static_cast<gid_t>(std::stoul(token)));
        }
    }
    return groups;
}

// Maps the lowercase name used on the CLI (e.g. "as") to this platform's RLIMIT_* constant.
std::optional<int> resource_from_name(const std::string& name)
{
    if (name == "as")
    {
        return RLIMIT_AS;
    }
    if (name == "core")
    {
        return RLIMIT_CORE;
    }
    if (name == "cpu")
    {
        return RLIMIT_CPU;
    }
    if (name == "data")
    {
        return RLIMIT_DATA;
    }
    if (name == "fsize")
    {
        return RLIMIT_FSIZE;
    }
    if (name == "nofile")
    {
        return RLIMIT_NOFILE;
    }
    if (name == "stack")
    {
        return RLIMIT_STACK;
    }
    return std::nullopt;
}

// Parse a single rlimit bound: either a non-negative integer, or the literal "unlimited", which
// maps to RLIM_INFINITY.
rlim_t parse_rlimit_bound(const std::string& token)
{
    if (token == "unlimited")
    {
        return RLIM_INFINITY;
    }
    return static_cast<rlim_t>(std::stoull(token));
}

// Parse "--rlimit-<name>=<soft>,<hard>" into an RlimitExpectation, or nullopt if 'arg' does not
// have this form or names an unrecognized resource.
std::optional<sandbox_options::RlimitExpectation> parse_rlimit_option(const std::string& arg)
{
    constexpr std::string_view kPrefix = "--rlimit-";
    if (arg.rfind(kPrefix, 0) != 0)
    {
        return std::nullopt;
    }

    const auto equals_pos = arg.find('=');
    if (equals_pos == std::string::npos)
    {
        return std::nullopt;
    }

    const std::string name = arg.substr(kPrefix.size(), equals_pos - kPrefix.size());
    const auto resource = resource_from_name(name);
    if (!resource.has_value())
    {
        return std::nullopt;
    }

    const std::string bounds = arg.substr(equals_pos + 1);
    const auto comma_pos = bounds.find(',');
    if (comma_pos == std::string::npos)
    {
        return std::nullopt;
    }

    std::string display_name = "RLIMIT_";
    for (const char c : name)
    {
        display_name += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    return sandbox_options::RlimitExpectation{
        *resource,
        display_name,
        parse_rlimit_bound(bounds.substr(0, comma_pos)),
        parse_rlimit_bound(bounds.substr(comma_pos + 1))};
}

// If 'arg' has the form "--<key>=<value>", store the value in 'value' and return true.
bool match_option(const std::string& arg, const std::string_view key, std::string& value)
{
    const std::string prefix = "--" + std::string(key) + "=";
    if (arg.rfind(prefix, 0) == 0)
    {
        value = arg.substr(prefix.size());
        return true;
    }
    return false;
}

// Populate 'out' from the command line arguments. Only the options that are actually present are
// set; any option left out stays unset and is therefore not verified. Returns false if an
// unrecognized option is encountered.
bool parse_arguments(int argc, char** argv, ExpectedValues& out)
{
    bool all_recognized = true;

    std::string value;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (auto rlimit = parse_rlimit_option(arg))
        {
            out.rlimits.push_back(std::move(*rlimit));
        }
        else if (match_option(arg, "uid", value))
        {
            out.uid = static_cast<uid_t>(std::stoul(value));
        }
        else if (match_option(arg, "gid", value))
        {
            out.gid = static_cast<gid_t>(std::stoul(value));
        }
        else if (match_option(arg, "supplementary-groups", value))
        {
            out.supplementary_groups = parse_groups(value);
        }
        else if (match_option(arg, "scheduling-policy", value))
        {
            out.policy = parse_policy(value);
        }
        else if (match_option(arg, "scheduling-priority", value))
        {
            out.priority = std::stoi(value);
        }
        else if (match_option(arg, "working-dir", value))
        {
            out.working_dir = value;
        }
        else if (match_option(arg, "affinity", value))
        {
            if (value == "all")
            {
                out.affinity = sandbox_options::AffinityExpectation{true, 0};
            }
            else
            {
                out.affinity = sandbox_options::AffinityExpectation{false, std::stoull(value, nullptr, 16)};
            }
        }
        else
        {
            std::cerr << "Unrecognized argument: " << arg << std::endl;
            all_recognized = false;
        }
    }

    // Ensure that, if we configure a process without any supplementary groups, it is launched without any
    if (!out.supplementary_groups.has_value())
    {
        out.supplementary_groups = std::vector<gid_t>{};
    }

    return all_recognized;
}

}  // namespace

TEST(SandboxOptions, RunAndVerify)
{
    // Each sandbox option is verified in its own step so the test log shows exactly what is checked.
    // An option that is left unset (see ExpectedValues) is not configured for this process and is
    // therefore skipped; scheduling is always verified against the launch manager's defaults.
    if (expected.uid.has_value() || expected.gid.has_value())
    {
        TEST_STEP("Verify uid and gid")
        {
            EXPECT_TRUE(sandbox_options::verifyUidGid(expected.uid, expected.gid));
        }
    }

    if (expected.supplementary_groups.has_value())
    {
        TEST_STEP("Verify supplementary groups")
        {
            EXPECT_TRUE(sandbox_options::verifySupplementaryGroups(*expected.supplementary_groups));
        }
    }

    if (expected.working_dir.has_value())
    {
        TEST_STEP("Verify working directory")
        {
            EXPECT_TRUE(sandbox_options::verifyWorkingDir(*expected.working_dir));
        }
    }

    if (expected.affinity.has_value())
    {
        TEST_STEP("Verify CPU affinity on main thread")
        {
            EXPECT_TRUE(
                expected.affinity->all_cpus ? sandbox_options::verifyAffinityAllCpus()
                                            : sandbox_options::verifyAffinity(expected.affinity->mask));
        }
        TEST_STEP("Verify CPU affinity on a spawned thread")
        {
            ::testing::AssertionResult thread_result = ::testing::AssertionSuccess();
            std::thread worker([&thread_result]() {
                thread_result = expected.affinity->all_cpus ? sandbox_options::verifyAffinityAllCpus()
                                                            : sandbox_options::verifyAffinity(expected.affinity->mask);
            });
            worker.join();
            EXPECT_TRUE(thread_result);
        }
    }

    if (!expected.rlimits.empty())
    {
        TEST_STEP("Verify resource limits")
        {
            for (const auto& rlimit_expectation : expected.rlimits)
            {
                EXPECT_TRUE(sandbox_options::verifyRlimit(rlimit_expectation));
            }
        }
    }

    TEST_STEP("Verify scheduling policy and priority in the main thread")
    {
        EXPECT_TRUE(sandbox_options::verifyScheduling(expected.policy, expected.priority, "main thread"));
    }

    TEST_STEP("Verify scheduling policy and priority in a spawned thread")
    {
        // A thread created with default attributes inherits the scheduling policy and priority of
        // its creating thread, so the configured settings must apply here as well. The result is
        // written in the worker and read after the join, which synchronizes the access.
        ::testing::AssertionResult thread_result = ::testing::AssertionSuccess();
        std::thread worker([&thread_result]() {
            thread_result = sandbox_options::verifyScheduling(expected.policy, expected.priority, "spawned thread");
        });
        worker.join();
        EXPECT_TRUE(thread_result);
    }
}

int main(int argc, char** argv)
{
    if (!parse_arguments(argc, argv, expected))
    {
        std::cerr << "Recognized sandbox options: --uid, --gid, --supplementary-groups, "
                     "--scheduling-policy, --scheduling-priority, --working-dir, --affinity, "
                     "--rlimit-<as|core|cpu|data|fsize|nofile|stack>=<soft>,<hard>"
                  << std::endl;
        return 1;
    }
    // Derive the GTest XML result file name from the executable name (argv[0]) rather than the
    // shared source file, so that the two binaries built from this source (sandbox_options_process_a
    // and sandbox_options_process_b) write distinct result files. Dereference rather than index to
    // avoid the no-pointer-arithmetic lint rule.
    const char* const executable_path = (argc > 0) ? *argv : __FILE__;
    return TestRunner(executable_path, TerminationBehavior::kContinue).RunTests();
}
