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
    name = "lm_process_fd_leak_config",
    config = "//tests/integration/process_fd_leak:process_fd_leak.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "native",
    srcs = [
        "get_fds.hpp",
        "native.cpp",
    ],
    deps = [
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "reporting",
    srcs = [
        "get_fds.hpp",
        "reporting.cpp",
    ],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "control_client",
    srcs = [
        "control_client.cpp",
        "get_fds.hpp",
    ],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log",
    ],
)

pkg_files(
    name = "main_files",
    srcs = [
        ":control_client",
        ":native",
        ":reporting",
        "//score/launch_manager",
    ],
    attributes = pkg_attributes(mode = "0777"),
    prefix = "tests/process_fd_leak",
)

pkg_files(
    name = "configs",
    srcs = [":lm_process_fd_leak_config"],
    prefix = "tests/process_fd_leak",
)

pkg_tar(
    name = "process_fd_leak_binaries",
    srcs = [
        ":configs",
        ":main_files",
    ],
)

integration_test(
    name = "process_fd_leak",
    srcs = ["process_fd_leak.py"],
    tags = ["integration"],
    test_binaries = ":process_fd_leak_binaries",
    deps = ["//tests/utils/testing_utils"],
)
