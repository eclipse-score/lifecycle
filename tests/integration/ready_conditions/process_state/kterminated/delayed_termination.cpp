/*******************************************************************************
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
 *******************************************************************************/

/*
 * ./delayed_termination --timeout-milliseconds <timeout in milliseconds>
 *
 * This process will wait until a configured millisecond timeout has been reached and then terminate.
 *
 * Exit codes:
 * - Exit code 0: Successfully completed the sleep timeout.
 * - Exit code 1: Usage, parsing, or system errors.
 */

#include <charconv>
#include <getopt.h>
#include <time.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <system_error>

namespace
{
/**
 * @brief Converts a C-string to an unsigned 64-bit integer.
 * @param str The null-terminated string to convert
 * @return The parsed uint64_t value, or std::nullopt if parsing fails
 */
[[nodiscard]] std::optional<std::uint64_t> cstr_to_uint64(const char* str) noexcept
{
    if (str == nullptr || *str == '\0')
    {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    const std::size_t len = std::strlen(str);
    const char* last = str + len;
    auto [ptr, ec] = std::from_chars(str, last, value);
    if (ec == std::errc{} && ptr == last)
    {
        return value;
    }
    return std::nullopt;
}

/**
 * @brief Pauses execution of the calling thread for the specified millisecond duration.
 * @param milliseconds The number of milliseconds to sleep
 * @return 0 on success, or 1 if an error occurred
 */
[[nodiscard]] int wait_for_timeout(const std::uint64_t milliseconds) noexcept
{
    // Convert total milliseconds to seconds and nanoseconds
    const time_t sec = static_cast<time_t>(milliseconds / 1000U);
    const long nsec = static_cast<long>((milliseconds % 1000U) * 1000000UL);

    struct timespec request{sec, nsec};
    struct timespec remaining{0, 0};

    std::printf("Starting countdown: waiting for %llu ms...\n", static_cast<unsigned long long>(milliseconds));

    // Handle potential signal interruptions during the sleep loop
    while (::nanosleep(&request, &remaining) != 0)
    {
        if (errno == EINTR)
        {
            // Interrupted by signal; resume sleeping for the remaining duration
            request = remaining;
        }
        else
        {
            std::fprintf(stderr, "delayed_termination: nanosleep failed: %s\n", std::strerror(errno));
            return 1;
        }
    }

    std::printf("Timeout reached. Exiting.\n");
    return 0;
}
}  // namespace

int main(int argc, char** argv)
{
    constexpr std::uint64_t DEFAULT_TIMEOUT_MILLISECONDS = 10000U;  // 10 seconds default
    bool help = false;
    std::uint64_t timeout_milliseconds = DEFAULT_TIMEOUT_MILLISECONDS;
    bool timeout_provided = false;

    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"timeout-milliseconds", required_argument, nullptr, 't'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "ht:", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'h':
                help = true;
                break;
            case 't':
                if (optarg)
                {
                    auto maybe_timeout = cstr_to_uint64(optarg);
                    if (maybe_timeout.has_value())
                    {
                        timeout_milliseconds = *maybe_timeout;
                        timeout_provided = true;
                    }
                    else
                    {
                        std::fprintf(stderr, "Could not parse timeout-milliseconds as integer: %s\n", optarg);
                        return 1;
                    }
                }
                break;
            case '?':
            default:
                std::fprintf(stderr, "Invalid or missing argument.\n");
                return 1;
        }
    }

    if (help)
    {
        std::printf("Usage: %s --timeout-milliseconds <timeout in ms>\n", argv[0]);
        return 0;
    }

    if (!timeout_provided)
    {
        std::fprintf(
            stderr,
            "Warning: No timeout value provided. Using default of %llu ms.\n",
            static_cast<unsigned long long>(DEFAULT_TIMEOUT_MILLISECONDS));
    }

    return wait_for_timeout(timeout_milliseconds);
}
