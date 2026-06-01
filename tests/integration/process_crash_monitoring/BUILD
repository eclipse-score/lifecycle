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
    name = "lm_process_crash_monitoring_config",
    config = "//tests/integration/process_crash_monitoring:process_crash_monitoring.json",
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

cc_binary(
    name = "process_crashing_on_runtime",
    srcs = ["process_crashing_on_runtime.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "process_crash_monitoring_main_files",
    srcs = [
        ":control_client_mock",
        ":process_crashing_on_runtime",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/process_crash_monitoring",
)

pkg_files(
    name = "process_crash_monitoring_etc_files",
    srcs = [":lm_process_crash_monitoring_config"],
    prefix = "tests/process_crash_monitoring",
)

pkg_tar(
    name = "process_crash_monitoring_binaries",
    srcs = [
        ":process_crash_monitoring_etc_files",
        ":process_crash_monitoring_main_files",
    ],
)

integration_test(
    name = "process_crash_monitoring",
    srcs = ["process_crash_monitoring.py"],
    tags = ["integration"],
    test_binaries = ":process_crash_monitoring_binaries",
    deps = ["//tests/utils/testing_utils"],
)
