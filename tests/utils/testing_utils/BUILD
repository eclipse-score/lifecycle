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
load("@rules_python//python:defs.bzl", "py_library")
load("@score_lifecycle_pip//:requirements.bzl", "requirement")

py_library(
    name = "testing_utils",
    srcs = [
        "run_until_file_deployed.py",
        "setup_test.py",
        "test_results.py",
    ],
    visibility = ["//tests:__subpackages__"],
    deps = [
        requirement("junitparser"),
        "@score_itf//score/itf/core/process",
        "@score_itf//score/itf/core/target",
    ],
)
