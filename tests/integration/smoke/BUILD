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

exports_files(
    ["lm_demo.json"],
    visibility = ["//examples:__subpackages__"],
)

launch_manager_config(
    name = "lm_smoketest_config",
    config = "//tests/integration/smoke:lifecycle_smoketest.json",
    flatbuffer_out_dir = "etc",
)

cc_binary(
    name = "control_daemon_mock",
    srcs = ["control_daemon_mock.cpp"],
    data = [],
    deps = [
        "//src/control_client_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "gtest_process",
    srcs = ["gtest_process.cpp"],
    deps = [
        "//src/control_client_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "smoke_main_files",
    srcs = [
        ":control_daemon_mock",
        ":gtest_process",
        "//src/launch_manager_daemon:launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/smoke",
)

pkg_files(
    name = "smoke_etc_files",
    srcs = [":lm_smoketest_config"],
    prefix = "tests/smoke",
)

pkg_tar(
    name = "smoke_binaries",
    srcs = [
        ":smoke_etc_files",
        ":smoke_main_files",
    ],
)

integration_test(
    name = "smoke",
    srcs = ["smoke.py"],
    tags = ["integration"],
    test_binaries = ":smoke_binaries",
    deps = ["//tests/utils/testing_utils"],
)
