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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_library(
    name = "shared_dependency_shutdown_common",
    hdrs = ["common.hpp"],
)

cc_binary(
    name = "control_client_test_driver",
    srcs = ["control_client_test_driver.cpp"],
    deps = [
        ":shared_dependency_shutdown_common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:shm_counter",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "component_shared_process",
    srcs = ["component_shared_process.cpp"],
    deps = [
        ":shared_dependency_shutdown_common",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:shm_counter",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "dependent_process",
    srcs = ["dependent_process.cpp"],
    deps = [
        ":shared_dependency_shutdown_common",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:arg_parser",
        "//tests/utils/test_helper:shm_counter",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "shared_dependency_shutdown",
    srcs = ["test.py"],
    binaries = [
        ":control_client_test_driver",
        ":component_shared_process",
        ":dependent_process",
        "//score/launch_manager",
    ],
    config = ":config.json",
)
