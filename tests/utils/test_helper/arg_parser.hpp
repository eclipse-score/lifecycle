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
#ifndef TESTS_UTILS_TEST_HELPER_ARG_PARSER_HPP
#define TESTS_UTILS_TEST_HELPER_ARG_PARSER_HPP

#include <cstdlib>
#include <string_view>
#include <tuple>

namespace score::mw::lifecycle::test
{

/// @brief Returns the `value` from "--name value" or "--name=value", nullptr if absent.
inline const char* FindArg(int argc, char** argv, std::string_view name)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};
        // --name value
        const bool flag_provided = arg == name;
        const bool value_provided = i + 1 < argc;
        if (flag_provided && value_provided)
        {
            return argv[i + 1];
        }
        // --name=value
        const bool flag_is_substr = arg.size() > name.size() && arg.substr(0, name.size()) == name;
        const bool eq_at_end = arg[name.size()] == '=';
        if (flag_is_substr && eq_at_end)
        {
            return argv[i] + name.size() + 1;
        }
    }
    return nullptr;
}

/// @brief Converts a raw argument string to T.
/// @details Add specializations for further types.
template <typename T>
T ConvertArg(const char* raw_value);

template <>
inline int ConvertArg<int>(const char* raw_value)
{
    return std::atoi(raw_value);
}

/// @brief Parses all given arguments, if a flag is not given the
///        `default_value` is used.
///
/// Usage:
///   struct arg
///   {
///       static constexpr std::string_view name = "--my-arg";
///       using type = int;
///       static constexpr type default_value = 0;
///   };
///   auto [start] = ParseArgs<StartPos>(argc, argv);
template <typename... Opts>
std::tuple<typename Opts::type...> ParseArgs(int argc, char** argv)
{
    return std::tuple<typename Opts::type...>{[&] {
        const char* raw_value = FindArg(argc, argv, Opts::name);
        if (raw_value == nullptr)
        {
            return Opts::default_value;
        }
        return ConvertArg<typename Opts::type>(raw_value);
    }()...};
}

}  // namespace score::mw::lifecycle::test

#endif  // TESTS_UTILS_TEST_HELPER_ARG_PARSER_HPP
