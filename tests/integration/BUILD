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
load("@pip_score_venv_test//:requirements.bzl", "all_requirements")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_tooling//python_basics:defs.bzl", "score_py_pytest", "score_virtualenv")

# In order to update the requirements, change the `requirements.txt` file and run:
# `bazel run //tests/integration:requirements.update`.
# This will update the `requirements.lock` file.
# To upgrade all dependencies to their latest versions, run:
# `bazel run //tests/integration:requirements.update -- --upgrade`.
compile_pip_requirements(
    name = "requirements",
    srcs = [
        "requirements.txt",
        "@score_tooling//python_basics:requirements.txt",
    ],
    extra_args = [
        "--no-annotate",
    ],
    requirements_txt = "requirements.lock",
    tags = [
        "manual",
    ],
)

score_virtualenv(
    name = "python_tc_venv",
    reqs = all_requirements,
    venv_name = ".python_tc_venv",
)

cc_library(
    name = "test_helper",
    hdrs = ["test_helper.hpp"],
    visibility = ["//tests:__subpackages__"],
    deps = [
        "@googletest//:gtest_main",
    ],
)

py_library(
    name = "control_interface",
    srcs = ["control_interface.py"],
    visibility = ["//tests:__subpackages__"],
)

py_library(
    name = "testing_utils",
    srcs = ["testing_utils.py"],
    visibility = ["//tests:__subpackages__"],
    deps = [":control_interface"],
)
