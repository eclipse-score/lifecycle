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
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//:defs.bzl", "launch_manager_config")
load("//tests/utils/bazel:integration.bzl", "integration_test")

launch_manager_config(
    name = "lm_process_wrong_binary_failure_config",
    config = "//tests/integration/process_wrong_binary_failure:process_wrong_binary_failure.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "control_client_mock",
    srcs = ["control_client_mock.cpp"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "process_wrong_binary_failure_main_files",
    srcs = [
        ":control_client_mock",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/process_wrong_binary_failure",
)

pkg_files(
    name = "process_wrong_binary_failure_etc_files",
    srcs = [":lm_process_wrong_binary_failure_config"],
    prefix = "tests/process_wrong_binary_failure",
)

pkg_tar(
    name = "process_wrong_binary_failure_binaries",
    srcs = [
        ":process_wrong_binary_failure_etc_files",
        ":process_wrong_binary_failure_main_files",
    ],
)

integration_test(
    name = "process_wrong_binary_failure",
    srcs = ["process_wrong_binary_failure.py"],
    tags = ["integration"],
    test_binaries = ":process_wrong_binary_failure_binaries",
    deps = ["//tests/utils/testing_utils"],
)
