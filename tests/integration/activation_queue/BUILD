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
load("@rules_cc//cc:cc_library.bzl", "cc_library")

exports_files(
    ["config.json"],
    visibility = ["//tests/integration/activation_queue:__subpackages__"],
)

cc_library(
    name = "test_fixture",
    hdrs = ["test_fixture.hpp"],
    visibility = ["//tests/integration/activation_queue:__subpackages__"],
    deps = [
        "//score/launch_manager:control_cc",
        "//score/launch_manager:lifecycle_cc",
        "//tests/utils/test_helper",
        "//tests/utils/test_helper:gated_process_helper",
        "//tests/utils/test_helper:test_runner",
        "@googletest//:gtest_main",
    ],
)
