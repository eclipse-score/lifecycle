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
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_lifecycle_pip//:requirements.bzl", "all_requirements")
load("@score_tooling//:defs.bzl", "score_py_pytest")
load("//:defs.bzl", "launch_manager_config")
load("//config:flatbuffers_rules.bzl", "flatbuffer_json_to_bin")
load("//tests/utils/bazel:integration.bzl", "integration_test")

launch_manager_config(
    name = "lm_non_reporting_config",
    config = ":non_reporting_config.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "non_reporting_process",
    srcs = ["non_reporting_process.cpp"],
    deps = [
        "//src/launch_manager_daemon/common:osal",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "non_reporting_main_files",
    srcs = [
        ":non_reporting_process",
        "//src/launch_manager_daemon:launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/incorrect_config_non_reporting",
)

pkg_files(
    name = "non_reporting_etc_files",
    srcs = [":lm_non_reporting_config"],
    prefix = "tests/incorrect_config_non_reporting",
)

pkg_tar(
    name = "non_reporting_binaries",
    srcs = [
        ":non_reporting_etc_files",
        ":non_reporting_main_files",
    ],
)

integration_test(
    name = "incorrect_config_non_reporting",
    srcs = ["test_incorrect_config_non_reporting.py"],
    tags = ["integration"],
    test_binaries = ":non_reporting_binaries",
    deps = ["//tests/utils/testing_utils"],
)
