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

load("@rules_python//python:defs.bzl", "py_binary")
load("@score_lifecycle_pip//:requirements.bzl", "requirement")

exports_files([
    "generate_health_monitor_schema.py",
    "health_monitor_config.py",
    "health_monitor_tests.py",
    "integration_tests.py",
    "unit_tests.py",
    "lifecycle_config.py",
])

py_binary(
    name = "lifecycle_config",
    srcs = ["lifecycle_config.py"],
    visibility = ["//visibility:public"],
    deps = [requirement("jsonschema")],
)

py_binary(
    name = "health_monitor_config",
    srcs = ["health_monitor_config.py"],
    visibility = ["//visibility:public"],
    deps = [requirement("jsonschema")],
)

# Regenerates the checked-in health monitor JSON schema from health_monitor_config.fbs.
#   bazel run //scripts/config_mapping:generate_health_monitor_schema
# The output is a workspace-relative path, not a label: the schema is written, not read,
# so the target also works when the schema file does not exist.
py_binary(
    name = "generate_health_monitor_schema",
    srcs = ["generate_health_monitor_schema.py"],
    args = [
        "--in",
        "$(location //score/health_monitor/configuration:health_monitor_config_postproc_schema)",
        "--output",
        "score/health_monitor/configuration/health_monitor_config_schema.json",
    ],
    data = [
        "//score/health_monitor/configuration:health_monitor_config_postproc_schema",
    ],
    target_compatible_with = ["@platforms//os:linux"],
)

filegroup(
    name = "pip_requirements",
    srcs = ["requirements.txt"],
    visibility = ["//:__pkg__"],
)
