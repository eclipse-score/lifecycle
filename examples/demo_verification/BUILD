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
    name = "examples_test_config",
    config = ":lifecycle_demo_test.json",
    flatbuffer_out_dir = "etc",
)

pkg_files(
    name = "examples_main_files",
    srcs = [
        "//examples/c_supervised_app",
        "//examples/control_application:control_daemon",
        "//examples/control_application:lmcontrol",
        "//examples/cpp_lifecycle_app",
        "//examples/cpp_supervised_app",
        "//examples/rust_supervised_app",
        "//score/launch_manager",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "tests/examples",
)

pkg_files(
    name = "examples_etc_files",
    srcs = [
        "ecu_logging_config.json",
        "hm_logging.json",
        "logging.json",
        ":examples_test_config",
    ],
    prefix = "tests/examples",
)

pkg_tar(
    name = "examples_binaries",
    srcs = [
        ":examples_etc_files",
        ":examples_main_files",
    ],
)

integration_test(
    name = "examples",
    srcs = ["test_examples.py"],
    test_binaries = ":examples_binaries",
    deps = ["//tests/utils/testing_utils"],
)
