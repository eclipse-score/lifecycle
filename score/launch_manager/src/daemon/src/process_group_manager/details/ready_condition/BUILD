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
    name = "iready_condition",
    hdrs = ["iready_condition.hpp"],
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
    name = "mock_ready_condition",
    testonly = True,
    hdrs = ["mock_ready_condition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iready_condition",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "file_ready_condition",
    srcs = ["file_ready_condition.cpp"],
    hdrs = ["file_ready_condition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/ready_condition",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iready_condition",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/configuration:component_config",
        "//score/launch_manager/src/daemon/src/osal:ifile_waiter",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "@score_baselibs//score/language/safecpp/string_view:zstring_view",
    ],
)

lm_cc_test(
    name = "file_ready_condition_UT",
    srcs = ["file_ready_condition_UT.cpp"],
    deps = [
        ":file_ready_condition",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:mock_ifile_waiter",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "report_running_ready_condition",
    srcs = ["report_running_ready_condition.cpp"],
    hdrs = ["report_running_ready_condition.hpp"],
    include_prefix = "score/mw/launch_manager/process_group_manager/details/ready_condition",
    strip_include_prefix = "/score/launch_manager/src/daemon/src/process_group_manager/details/ready_condition",
    visibility = ["//score/launch_manager/src/daemon/src/process_group_manager:__subpackages__"],
    deps = [
        ":iready_condition",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/osal:return_types",
        "//score/launch_manager/src/daemon/src/process_group_manager:iprocess",
    ],
)

lm_cc_test(
    name = "report_running_ready_condition_UT",
    srcs = ["report_running_ready_condition_UT.cpp"],
    deps = [
        ":report_running_ready_condition",
        "//score/launch_manager:error",
        "//score/launch_manager/src/daemon/src/process_group_manager:mock_iprocess",
        "@googletest//:gtest_main",
    ],
)
