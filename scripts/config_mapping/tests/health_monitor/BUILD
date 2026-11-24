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

load("@score_lifecycle_pip//:requirements.bzl", "requirement")
load("@score_tooling//:defs.bzl", "score_py_pytest")

score_py_pytest(
    name = "health_monitor_tests",
    srcs = [
        "conftest.py",
        "//scripts/config_mapping:health_monitor_tests.py",
    ],
    args = [
        "--generated-schema=$(location //score/health_monitor/configuration:health_monitor_config_postproc_schema)",
        "--schema-file=$(location //score/health_monitor/configuration:health_monitor_config_schema.json)",
        "--example=$(location //score/health_monitor/configuration:example/health_monitor_config.json)",
    ],
    data = [
        "//score/health_monitor/configuration:example/health_monitor_config.json",
        "//score/health_monitor/configuration:health_monitor_config_postproc_schema",
        "//score/health_monitor/configuration:health_monitor_config_schema.json",
        "//scripts/config_mapping:generate_health_monitor_schema.py",
        "//scripts/config_mapping:health_monitor_config.py",
    ] + glob(["invalid/*.json"]),
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        requirement("pytest"),
        requirement("jsonschema"),
    ],
)
