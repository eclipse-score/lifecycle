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
load("//tests/utils/bazel:integration.bzl", "integration_test")

integration_test(
    name = "component_recovery_log",
    srcs = ["component_recovery_log.py"],
    binaries = [
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":component_recovery_log.json",
)
