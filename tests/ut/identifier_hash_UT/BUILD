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

load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "identifier_hash_UT",
    srcs = ["identifier_hash_UT.cpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        "//src/launch_manager_daemon/common:identifier_hash",
        "@googletest//:gtest_main",
    ],
)
