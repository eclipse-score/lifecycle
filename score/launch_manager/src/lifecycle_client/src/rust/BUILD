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
load("@rules_rust//rust:defs.bzl", "rust_library")
load("//tests/utils/bazel:unit_test.bzl", "lm_rust_test")

rust_library(
    name = "lifecycle_client_rs",
    srcs = [
        "src/lib.rs",
        "src/lifecycle.rs",
    ],
    visibility = ["//score/launch_manager:__subpackages__"],
    deps = [
        "//score/launch_manager/src/lifecycle_client:report_running",
        "@score_crates//:libc",
    ],
)

lm_rust_test(
    name = "lifecycle_client_rs_test",
    crate = ":lifecycle_client_rs",
)
