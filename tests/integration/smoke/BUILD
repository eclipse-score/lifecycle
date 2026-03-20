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
load("@score_itf//:defs.bzl", "py_itf_test")
load("//config:flatbuffers_rules.bzl", "flatbuffer_json_to_bin")
load("//tests/utils/bazel:integration.bzl", "integration_test")
load("//tests/utils/bazel:package_test_binaries.bzl", "package_test_binaries")

flatbuffer_json_to_bin(
    name = "test_lm_cfg",
    json = "lm_demo.json",
    schema = "//src/launch_manager_daemon:config/lm_flatcfg.fbs",
)

flatbuffer_json_to_bin(
    name = "test_hm_cfg",
    json = "hm_demo.json",
    schema = "//src/launch_manager_daemon/health_monitor_lib:config/hm_flatcfg.fbs",
)

cc_binary(
    name = "control_daemon_mock",
    srcs = ["control_daemon_mock.cpp"],
    data = [
        ":test_hm_cfg",
        ":test_lm_cfg",
    ],
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

package_test_binaries(
    name = "smoke_binaries",
    test_name = "smoke",
    binaries = {
        "//src/launch_manager_daemon:launch_manager": "",
        ":control_daemon_mock": "",
        ":gtest_process": "",
        ":test_lm_cfg": "etc",
        ":test_hm_cfg": "etc",
    },
)

integration_test(
    name = "smoke",
    srcs = ["smoke.py"],
    test_binaries = ":smoke_binaries",
    deps = ["//tests/utils/testing_utils"],
    tags = ["integration"],
)
