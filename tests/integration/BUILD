load("@rules_python//python:pip.bzl", "compile_pip_requirements")

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
load("@score_tooling//python_basics:defs.bzl", "score_virtualenv")

filegroup(
    name = "pip_requirements",
    srcs = ["requirements.txt"],
    visibility = ["//:__pkg__"],
)

# Only pass packages that don't overlap with pip_tooling (which already
# provides: iniconfig, packaging, pluggy, pygments, pytest).
score_virtualenv(
    name = "python_tc_venv",
    reqs = [
        requirement("junitparser"),
    ],
    venv_name = ".python_tc_venv",
)
