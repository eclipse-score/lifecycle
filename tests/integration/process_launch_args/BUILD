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
    name = "lm_process_launch_args_config",
    config = "//tests/integration/process_launch_args:process_launch_args.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "process_initial",
    srcs = ["process_initial.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "process_launch_args_main_files",
    srcs = [
        ":process_initial",
        "//score/launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/process_launch_args",
)

pkg_files(
    name = "process_launch_args_etc_files",
    srcs = [":lm_process_launch_args_config"],
    prefix = "tests/process_launch_args",
)

pkg_tar(
    name = "process_launch_args_binaries",
    srcs = [
        ":process_launch_args_etc_files",
        ":process_launch_args_main_files",
    ],
)

integration_test(
    name = "process_launch_args",
    srcs = ["process_launch_args.py"],
    tags = ["integration"],
    test_binaries = ":process_launch_args_binaries",
    deps = ["//tests/utils/testing_utils"],
)
