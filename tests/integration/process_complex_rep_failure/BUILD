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
    name = "lm_process_complex_rep_failure_config",
    config = "//tests/integration/process_complex_rep_failure:process_complex_rep_failure.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "control_client_mock",
    srcs = ["control_client_mock.cpp"],
    deps = [
        "//src/control_client_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "complex_reporting_process",
    srcs = ["//tests/utils/test_helper:complex_reporting_process.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//src/lifecycle_client_lib",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "process_complex_rep_failure_main_files",
    srcs = [
        ":complex_reporting_process",
        ":control_client_mock",
        "//src/launch_manager_daemon:launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/process_complex_rep_failure",
)

pkg_files(
    name = "process_complex_rep_failure_etc_files",
    srcs = [":lm_process_complex_rep_failure_config"],
    prefix = "tests/process_complex_rep_failure",
)

pkg_tar(
    name = "process_complex_rep_failure_binaries",
    srcs = [
        ":process_complex_rep_failure_etc_files",
        ":process_complex_rep_failure_main_files",
    ],
)

integration_test(
    name = "process_complex_rep_failure",
    srcs = ["process_complex_rep_failure.py"],
    tags = ["integration"],
    test_binaries = ":process_complex_rep_failure_binaries",
    deps = ["//tests/utils/testing_utils"],
)
