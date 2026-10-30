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
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_library(
    name = "common",
    hdrs = ["common.hpp"],
)

cc_binary(
    name = "control_client_test_driver",
    srcs = ["control_client_test_driver.cpp"],
    deps = [
        ":common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "lm_control_requester",
    srcs = ["lm_control_requester.cpp"],
    deps = [
        ":common",
        "//score/launch_manager:control_cc",
    ],
)

integration_test(
    name = "lm_shutdown_with_control_requests",
    timeout = "short",
    srcs = ["test.py"],
    binaries = [
        "//tests/utils/test_helper:process_hanging_on_sigterm",
        ":control_client_test_driver",
        ":lm_control_requester",
        "//score/launch_manager",
    ],
    config = ":config.json",
)
