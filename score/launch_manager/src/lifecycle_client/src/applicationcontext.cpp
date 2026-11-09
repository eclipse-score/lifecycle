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

#include "score/mw/lifecycle/applicationcontext.h"
#include <algorithm>
#include <iterator>
#include <string_view>

score::mw::lifecycle::ApplicationContext::ApplicationContext(
    const std::int32_t argc,
    const char* const argv[])  // NOLINT(modernize-avoid-c-arrays): array tolerated for command line arguments
    : args_(argv, argv + argc), app_path_(argv[0])  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic): array
                                                    // tolerated for command line arguments
{
}

const std::vector<std::string>& score::mw::lifecycle::ApplicationContext::get_arguments() const noexcept
{
    return args_;
}

std::string score::mw::lifecycle::ApplicationContext::get_argument(const std::string_view flag) const noexcept
{
    std::string cont_result{""};
    if (!args_.empty())
    {
        const auto result = std::find_if(args_.begin(), args_.end(), [flag](const auto& arg) -> bool {
            return std::string_view{arg} == flag;
        });
        if ((result != args_.end()) && (std::next(result) != args_.end()))
        {
            cont_result = *std::next(result);
        }
    }

    return cont_result;
}
