# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "health_monitoring_lib",
    srcs = glob(["src/**/*.rs"]),
    crate_name = "health_monitoring_lib",
    proc_macro_deps = [
        "@score_baselibs_rust//src/testing_macros:score_testing_macros",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs_rust//src/log/score_log",
    ],
)

rust_test(
    name = "tests",
    crate = ":health_monitoring_lib",
    deps = [
        "@score_baselibs_rust//src/log/stdout_logger",
    ],
)
