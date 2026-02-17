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
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker", "rust_coverage_report", "setup_starpls", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        "examples",
        "src",
        "//:BUILD",
        "//:MODULE.bazel",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
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

# Rust coverage report generation target
rust_coverage_report(
    name = "rust_coverage",
    bazel_configs = [
        "x86_64-linux",
        "ferrocene-coverage",
    ],
    query = 'kind("rust_test", //src/...)',
    visibility = ["//visibility:public"],
)

alias(
    name = "rust_coverage_report",
    actual = ":rust_coverage",
    visibility = ["//visibility:public"],
)

# Docs
docs(
    data = [
        "@score_process//:needs_json",  # This allows linking to requirements (wp__requirements_comp, etc.) from the process_description repository.
    ],
    source_dir = "docs",
)
