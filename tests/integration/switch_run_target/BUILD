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
    name = "lm_switch_run_target_config",
    config = "//tests/integration/switch_run_target:switch_run_target.json",
    flatbuffer_out_dir = "etc",
)

cc_library(
    name = "switch_run_target_common",
    hdrs = ["common.hpp"],
)

cc_binary(
    name = "control_client_mock",
    srcs = ["control_client_mock.cpp"],
    deps = [
        ":switch_run_target_common",
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

[
    cc_binary(
        name = component,
        srcs = [component + ".cpp"],
        deps = [
            ":switch_run_target_common",
            "//score/launch_manager:lifecycle_cc",
            "//tests/utils/test_helper",
            "@googletest//:gtest_main",
        ],
    )
    for component in [
        "component_a",
        "component_b",
        "component_d",
    ]
]

cc_binary(
    name = "component_e",
    srcs = ["//tests/utils/test_helper:reporting_process.cpp"],
    deps = [
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

pkg_files(
    name = "switch_run_target_main_files",
    srcs = [
        ":component_a",
        ":component_b",
        ":component_d",
        ":component_e",
        ":control_client_mock",
        "//score/launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/switch_run_target",
)

pkg_files(
    name = "switch_run_target_etc_files",
    srcs = [":lm_switch_run_target_config"],
    prefix = "tests/switch_run_target",
)

pkg_tar(
    name = "switch_run_target_binaries",
    srcs = [
        ":switch_run_target_etc_files",
        ":switch_run_target_main_files",
    ],
)

integration_test(
    name = "switch_run_target",
    srcs = ["switch_run_target.py"],
    tags = ["integration"],
    test_binaries = ":switch_run_target_binaries",
    deps = ["//tests/utils/testing_utils"],
)
