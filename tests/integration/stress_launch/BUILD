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
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_python//python:defs.bzl", "py_binary")
load("//tests/utils/bazel:integration.bzl", "integration_test")

py_binary(
    name = "generate_config",
    srcs = ["generate_config.py"],
    target_compatible_with = ["@platforms//os:linux"],
)

cc_binary(
    name = "control_client_test_driver",
    srcs = ["control_client_test_driver.cpp"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "@googletest//:gtest_main",
    ],
)

# The configuration is generated, because it has one component per stress process.
genrule(
    name = "config",
    outs = ["config.json"],
    cmd = "$(location :generate_config) --name stress_launch --output $@",
    tools = [":generate_config"],
)

# The default QEMU config has 1G of RAM, which is not enough for 1024 processes on QNX.
# Use the QEMU config of the selected platform with more RAM.
genrule(
    name = "qemu_config",
    srcs = ["//config:qemu_config"],
    outs = ["qemu_config.json"],
    cmd = "sed 's/\"qemu_ram_size\": \"[^\"]*\"/\"qemu_ram_size\": \"2G\"/' $< > $@",
)

integration_test(
    name = "stress_launch",
    srcs = ["test.py"],
    binaries = [
        ":control_client_test_driver",
        "//score/launch_manager",
        "//tests/utils/test_helper:counting_process",
    ],
    config = ":config.json",
    qemu_config = ":qemu_config.json",
    # With TSan every process needs about 16 MB of RAM, which is too much for 1024 processes on the CI runners.
    target_compatible_with = ["@score_cpp_policies//sanitizers/constraints:no_tsan"],
)
