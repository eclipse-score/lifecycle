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
load("@score_tooling//:defs.bzl", "score_py_pytest")
load("//tests/utils:bazel/integration.bzl", "integration_test")

integration_test(
    name = "smoke",
    srcs = [
        "smoke.py",
    ],
    deps = [
        "//tests/utils/fixtures",
    ],
    args = [ "--image-path=$(location //tests/utils/environments:test_environment)" ],
    data = ["//tests/utils/environments:test_environment"],
    tags = ["local"]
)
