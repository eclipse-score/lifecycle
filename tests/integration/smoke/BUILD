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
load("@score_lifecycle_pip//:requirements.bzl", "all_requirements")
load("@score_tooling//:defs.bzl", "score_py_pytest")
load("//:defs.bzl", "launch_manager_config")
load("//config:flatbuffers_rules.bzl", "flatbuffer_json_to_bin")

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
        "//tests/integration:test_helper",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "gtest_process",
    srcs = ["gtest_process.cpp"],
    deps = [
        "//src/control_client_lib",
        "//src/launch_manager_daemon/lifecycle_client_lib:lifecycle_client",
        "//tests/integration:test_helper",
        "@googletest//:gtest_main",
    ],
)

score_py_pytest(
    name = "smoke",
    srcs = [
        "smoke.py",
    ],
    args = [
    ],
    data = [
        ":control_daemon_mock",
        ":gtest_process",
        ":lm_smoketest_config",
        "//src/launch_manager_daemon:launch_manager",
    ],
    tags = [
        "integration",
    ],
    deps = all_requirements + [
        "//tests/integration:testing_utils",
    ],
)
