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
load("//tests/utils/bazel:integration.bzl", "integration_test")

cc_binary(
    name = "cpp_alive_process",
    srcs = ["cpp_alive_process.cpp"],
    deps = [
        "//score/launch_manager:alive_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper:arg_parser",
    ],
)

integration_test(
    name = "cpp_alive",
    srcs = ["cpp_alive.py"],
    binaries = [
        ":cpp_alive_process",
        "//score/launch_manager",
        "//tests/integration/alive:control_client_test_driver",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":cpp_alive.json",
)
