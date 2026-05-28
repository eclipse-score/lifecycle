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
    name = "lm_crash_on_startup_config",
    config = "//tests/integration/crash_on_startup:crash_on_startup.json",
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
    name = "process_crashing_on_startup_twice",
    srcs = ["process_crashing_on_startup_twice.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "process_crashing_on_startup_always",
    srcs = ["process_crashing_on_startup_always.cpp"],
)

pkg_files(
    name = "crash_on_startup_main_files",
    srcs = [
        ":control_client_mock",
        ":process_crashing_on_startup_always",
        ":process_crashing_on_startup_twice",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/crash_on_startup",
)

pkg_files(
    name = "crash_on_startup_etc_files",
    srcs = [":lm_crash_on_startup_config"],
    prefix = "tests/crash_on_startup",
)

pkg_tar(
    name = "crash_on_startup_binaries",
    srcs = [
        ":crash_on_startup_etc_files",
        ":crash_on_startup_main_files",
    ],
)

integration_test(
    name = "crash_on_startup",
    srcs = ["crash_on_startup.py"],
    tags = ["integration"],
    test_binaries = ":crash_on_startup_binaries",
    deps = ["//tests/utils/testing_utils"],
)
