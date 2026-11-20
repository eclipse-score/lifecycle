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
        "//tests/utils/fixtures:activation",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:gated_process_helper",
        "@googletest//:gtest",
    ],
)

integration_test(
    name = "forced_activation",
    srcs = ["test.py"],
    binaries = [
        ":control_client_test_driver",
        "//tests/utils/test_helper:reporting_process",
        "//tests/utils/test_helper:gated_process",
        "//score/launch_manager",
        "//tests/utils/test_helper:fail_test",
    ],
    config = ":config.json",
)
