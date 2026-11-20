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
    name = "alive_success",
    srcs = ["alive_success.py"],
    binaries = [
        "//tests/integration/alive:cpp_alive_process",
        "//tests/integration/alive:rust_alive_process",
        "//score/launch_manager",
        "//tests/utils/test_helper:fail_test",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":alive_success.json",
)
