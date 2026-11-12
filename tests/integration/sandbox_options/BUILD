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
load("@rules_cc//cc:defs.bzl", "cc_binary")

# Shared by the root (//tests/integration/sandbox_options/root) and non-root
# (//tests/integration/sandbox_options/non_root) variants of the sandbox options test.
exports_files(
    ["sandbox_options.in.json"],
    visibility = ["//tests/integration/sandbox_options:__subpackages__"],
)

# Three binaries are built from the same source. Each verifies the sandbox options passed to it on
# the command line (see sandbox_options.in.json), and derives its GTest XML result file name from its
# own executable name so the results do not collide.
[
    cc_binary(
        name = "sandbox_options_process_{}".format(suffix),
        srcs = [
            "sandbox_options_process.cpp",
            "verify_sandbox.hpp",
        ],
        visibility = ["//tests/integration/sandbox_options:__subpackages__"],
        deps = [
            "//score/launch_manager:control_cc",
            "//score/launch_manager:lifecycle_cc",
            "//tests/utils/test_helper",
            "@googletest//:gtest_main",
        ],
    )
    for suffix in ("a", "b", "c")
]
