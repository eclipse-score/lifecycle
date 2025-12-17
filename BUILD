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

load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker", "setup_starpls", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        "config",
        "demo",
        "health_monitor",
        "launch_manager",
        "rust_bindings",
        "src",
        "//:BUILD",
        "//:MODULE.bazel",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "build_all",
    srcs = [
        "//health_monitor",
        "//health_monitor:hm_shared_lib",
        "//launch_manager",
        "//launch_manager:control_client",
        "//launch_manager:lifecycle_client",
        "//launch_manager:process_state_client",
        "//rust_bindings/lifecycle_client_rs",
        "//rust_bindings/monitor_rs",
        "//src/health_monitoring_lib",
    ],
)

# Needed for Dash tool to check python dependency licenses.
filegroup(
    name = "cargo_lock",
    srcs = [
        "Cargo.lock",
    ],
    visibility = ["//visibility:public"],
)

dash_license_checker(
    src = "//:cargo_lock",
    file_type = "",  # let it auto-detect based on project_config
    project_config = PROJECT_CONFIG,
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets()

docs(
    data = [
        "@score_process//:needs_json",  # This allows linking to requirements (wp__requirements_comp, etc.) from the process_description repository.
    ],
    source_dir = "docs",
)

# Test suites
test_suite(
    name = "unit_tests",
    testonly = True,
    tests = [
    ],
)
