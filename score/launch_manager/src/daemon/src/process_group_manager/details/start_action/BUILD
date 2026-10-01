# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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

load("//tests/utils/bazel:unit_test.bzl", "lm_cc_test")

cc_library(
    name = "istart_action",
    hdrs = ["istart_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/daemon/src/process_group_manager/details:handle",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "mock_start_action",
    testonly = True,
    hdrs = ["mock_start_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":istart_action",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "empty_start_action",
    srcs = ["empty_start_action.cpp"],
    hdrs = ["empty_start_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/start_action",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/start_action",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [":istart_action"],
)

lm_cc_test(
    name = "empty_start_action_UT",
    srcs = ["empty_start_action_UT.cpp"],
    deps = [
        ":empty_start_action",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "process_start_action",
    srcs = ["process_start_action.cpp"],
    hdrs = ["process_start_action.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/start_action",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/start_action",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":istart_action",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

lm_cc_test(
    name = "process_start_action_UT",
    srcs = ["process_start_action_UT.cpp"],
    deps = [
        ":process_start_action",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_iprocess",
        "@googletest//:gtest_main",
    ],
)
