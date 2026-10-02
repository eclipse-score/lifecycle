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
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_binary(
    name = "control_client_test_driver",
    srcs = ["control_client_test_driver.cpp"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:gated_process_helper",
        "@googletest//:gtest_main",
    ],
)

integration_test(
    name = "replaced_activation",
    timeout = "short",
    srcs = ["replaced_activation.py"],
    binaries = [
        ":control_client_test_driver",
        "//tests/utils/test_helper:gated_process",
        "//score/launch_manager",
    ],
    config = ":replaced_activation.json",
)
