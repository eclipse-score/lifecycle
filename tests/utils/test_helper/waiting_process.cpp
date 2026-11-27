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
/*
 * ./wait_for_semaphore --semaphore-name <semaphore_name> --expected-count <expected_count> [--timeout-seconds <timeout
 * in seconds>] [--start-from-zero] [--succeed-on-timeout]
 *
 * This process will wait for a named semaphore to reach an expected count, with an optional timeout.
 * We will also optionally clear/delete the semaphore on start.
 * This process can be used with the `counting_process` in order to wait for an expected number of processes to start.
 *
 * Exit codes:
 * - Exit code 0: all `expected_count` posts were observed
 *   (or --succeed-on-timeout was set, and we timed out).
 * - Exit code 1: usage error.
 * - Exit code 2: timed out before `expected_count` posts were observed; the
 *   number of posts actually observed so far is printed to stderr.
 */

#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include "tests/utils/test_helper/arg_parser.hpp"

namespace
{
using score::mw::lifecycle::test::ParseArgs;

struct HelpArg
{
    static constexpr std::string_view name = "--help";
    using type = bool;
};

struct SemaphoreNameArg
{
    static constexpr std::string_view name = "--semaphore-name";
    using type = std::string_view;
    static constexpr type default_value{};
};

struct ExpectedCountArg
{
    static constexpr std::string_view name = "--expected-count";
    using type = int;
    static constexpr type default_value = 0;
};

struct TimeoutSecondsArg
{
    static constexpr std::string_view name = "--timeout-seconds";
    using type = int;
    static constexpr type default_value = 10;
};

struct StartFromZeroArg
{
    static constexpr std::string_view name = "--start-from-zero";
    using type = bool;
};

struct SucceedOnTimeoutArg
{
    static constexpr std::string_view name = "--succeed-on-timeout";
    using type = bool;
};

/**
 * @brief Adds seconds to a timespec.
 * @param ts Pointer to the timespec structure to modify
 * @param seconds Number of seconds to add
 */
void add_seconds(timespec* ts, std::uint64_t seconds)
{
    ts->tv_sec += static_cast<time_t>(seconds);
}

/**
 * @brief Waits for a named semaphore to reach an expected count within a timeout period.
 * @param semaphore_name Name of the POSIX named semaphore to wait on
 * @param expected_count Number of posts to wait for
 * @param timeout_seconds Maximum time to wait in seconds
 * @return 0 if expected_count posts were observed, 1 on error, 2 on timeout
 */
[[nodiscard]] int wait_for_named_semaphore(
    const std::string& semaphore_name,
    const std::uint64_t expected_count,
    const std::uint64_t timeout_seconds,
    const bool succeed_on_timeout)
{
    sem_t* const semaphore = ::sem_open(semaphore_name.c_str(), O_CREAT, 0666, 0);
    if (semaphore == SEM_FAILED)
    {
        std::fprintf(
            stderr, "wait_for_semaphore: sem_open(\"%s\") failed: %s\n", semaphore_name.c_str(), std::strerror(errno));
        return 1;
    }

    timespec deadline{};
    if (::clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    {
        std::fprintf(stderr, "wait_for_semaphore: clock_gettime failed: %s\n", std::strerror(errno));
        static_cast<void>(::sem_close(semaphore));
        return 1;
    }
    add_seconds(&deadline, timeout_seconds);

    std::uint64_t observed = 0;
    while (observed < expected_count)
    {
        if (::sem_timedwait(semaphore, &deadline) != 0)
        {
            if (errno == ETIMEDOUT)
            {
                std::fprintf(
                    stderr,
                    "wait_for_semaphore: timed out after %llus waiting for \"%s\" to reach "
                    "%llu (observed %llu)\n",
                    static_cast<unsigned long long>(timeout_seconds),
                    semaphore_name.c_str(),
                    static_cast<unsigned long long>(expected_count),
                    static_cast<unsigned long long>(observed));
                static_cast<void>(::sem_close(semaphore));

                if (succeed_on_timeout)
                {
                    return 0;
                }

                return 2;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else
            {
                std::fprintf(stderr, "wait_for_semaphore: sem_timedwait failed: %s\n", std::strerror(errno));
                static_cast<void>(::sem_close(semaphore));
                return 1;
            }
        }
        ++observed;
    }

    static_cast<void>(::sem_close(semaphore));
    static_cast<void>(::sem_unlink(semaphore_name.c_str()));

    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    const auto [help, semaphore_arg, expected_count_arg, timeout_seconds_arg, start_from_zero, succeed_on_timeout] =
        ParseArgs<
            HelpArg,
            SemaphoreNameArg,
            ExpectedCountArg,
            TimeoutSecondsArg,
            StartFromZeroArg,
            SucceedOnTimeoutArg>(argc, argv);
    const std::string semaphore_name{semaphore_arg};

    if (help)
    {
        std::printf(
            "Usage: ./wait_for_semaphore --semaphore-name <semaphore_name> --expected-count <expected_count> "
            "[--timeout-seconds <timeout in seconds>] [--start-from-zero]\n");
        return 0;
    }

    if (semaphore_name.empty())
    {
        std::fprintf(stderr, "A semaphore name must be provided.\n");
        return 1;
    }

    if (expected_count_arg <= 0)
    {
        std::fprintf(stderr, "Invalid count! An expected count greater than zero must be provided.\n");
        return 1;
    }

    if (timeout_seconds_arg < 0)
    {
        std::fprintf(stderr, "Invalid timeout! The timeout must not be negative.\n");
        return 1;
    }

    if (semaphore_name[0] != '/')
    {
        std::fprintf(stderr, "Invalid semaphore name! POSIX named semaphores must start with '/'\n");
        return 1;
    }

    if (start_from_zero)
    {
        static_cast<void>(::sem_unlink(semaphore_name.c_str()));
    }

    return wait_for_named_semaphore(
        semaphore_name,
        static_cast<std::uint64_t>(expected_count_arg),
        static_cast<std::uint64_t>(timeout_seconds_arg),
        succeed_on_timeout);
}
