# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
load("//config:common_cc.bzl", "cc_binary_with_common_opts", "cc_library_with_common_opts")

cc_library(
    name = "identifier_hash",
    srcs = [
        "src/identifier_hash.cpp",
    ],
    hdrs = [
        "include/score/lcm/identifier_hash.hpp",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "common",
    srcs = [
        "src/internal/config.cpp",
    ],
    hdrs = [
        "include/score/lcm/internal/config.hpp",
        "include/score/lcm/internal/process_group_state_id.hpp",
    ],
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
    deps = [":identifier_hash"],
)

cc_library(
    name = "lifecycle_error",
    hdrs = [
        "include/score/lcm/exec_error_domain.h",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "log",
    hdrs = ["include/score/lcm/internal/log.hpp"],
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
)

cc_library(
    name = "osal",
    srcs = select({
        "//config:x86_64-qnx": glob(
            [
                "src/internal/osal/**/*.cpp",
            ],
            exclude = [
                "src/internal/osal/linux/**",
            ],
        ),
        "//config:build_qnx8": glob(
            [
                "src/internal/osal/**/*.cpp",
            ],
            exclude = [
                "src/internal/osal/linux/**",
            ],
        ),
        "//conditions:default": glob(
            [
                "src/internal/osal/**/*.cpp",
            ],
            exclude = [
                "src/internal/osal/qnx/**",
            ],
        ),
    }),
    hdrs = glob(
        ["include/score/lcm/internal/osal/**/*.hpp"],
        allow_empty = False,
    ),
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
    deps = [
        ":common",
        ":log",
    ],
)

cc_library(
    name = "control_client_channel",
    srcs = [
        "src/internal/controlclientchannel.cpp",
    ],
    hdrs = [
        "include/score/lcm/internal/controlclientchannel.hpp",
    ],
    includes = ["include"],
    visibility = ["//src:__subpackages__"],
    deps = [
        ":common",
        ":osal",
    ],
)
