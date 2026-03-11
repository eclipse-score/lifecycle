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

#include "src/lifecycle_client_lib/test/ut/mocks/applicationcontextmock.h"
#include "src/lifecycle_client_lib/include/applicationcontext.h"

#include <functional>

namespace
{

auto& GetConstructorCallback() noexcept
{
    static std::function<void(const std::int32_t argc, const score::StringLiteral argv[])> constructor_callback{};
    return constructor_callback;
}

auto& GetGetArgumentsCallback() noexcept
{
    static std::function<const std::vector<std::string>&()> get_arguments_callback{};
    return get_arguments_callback;
}

auto& GetGetArgumentCallback() noexcept
{
    static std::function<const std::string(const std::string_view)> get_argument_callback{};
    return get_argument_callback;
}

}  // namespace

score::mw::lifecycle::ApplicationContextMock::ApplicationContextMock()
{
    GetConstructorCallback() = [this](const std::int32_t argc, const score::StringLiteral argv[]) {
        ctor(argc, argv);
    };
    GetGetArgumentsCallback() = [this]() -> decltype(auto) {
        return get_arguments();
    };
    GetGetArgumentCallback() = [this](const std::string_view flag) -> decltype(auto) {
        return get_argument(flag);
    };
}

score::mw::lifecycle::ApplicationContextMock::~ApplicationContextMock()
{
    GetConstructorCallback() = nullptr;
    GetGetArgumentsCallback() = nullptr;
    GetGetArgumentCallback() = nullptr;
}

score::mw::lifecycle::ApplicationContext::ApplicationContext(const std::int32_t argc, const score::StringLiteral argv[])
{
    auto& constructor_callback = GetConstructorCallback();
    constructor_callback(argc, argv);
}

const std::vector<std::string>& score::mw::lifecycle::ApplicationContext::get_arguments() const noexcept
{
    auto& get_arguments_callback = GetGetArgumentsCallback();
    return get_arguments_callback();
}

std::string score::mw::lifecycle::ApplicationContext::get_argument(const std::string_view flag) const noexcept
{
    auto& get_argument_callback = GetGetArgumentCallback();
    return get_argument_callback(flag);
}
