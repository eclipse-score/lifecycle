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
load("@bazel_skylib//rules:expand_template.bzl", "expand_template")
load("//tests/utils/bazel:integration.bzl", "integration_test")

# UID to start lcm with, given to test in env vars.
_NON_ROOT_ID = "1000"

expand_template(
    name = "sandbox_options_non_root_config",
    out = "sandbox_options_non_root.json",
    substitutions = {
        '"uid": 0,': '"uid": {},'.format(_NON_ROOT_ID),
        '"gid": 0,': '"gid": {},'.format(_NON_ROOT_ID),
        '"--uid=0"': '"--uid={}"'.format(_NON_ROOT_ID),
        '"--gid=0"': '"--gid={}"'.format(_NON_ROOT_ID),
        # The test is installed under its own name, so paths into the install directory change too.
        '/tmp/tests/sandbox_options"': '/tmp/tests/sandbox_options_non_root"',
    },
    template = "//tests/integration/sandbox_options:sandbox_options.in.json",
)

integration_test(
    name = "sandbox_options_non_root",
    timeout = "short",
    srcs = ["sandbox_options_non_root.py"],
    binaries = [
        "//tests/integration/sandbox_options:sandbox_options_process_a",
        "//tests/integration/sandbox_options:sandbox_options_process_b",
        "//tests/integration/sandbox_options:sandbox_options_process_c",
        "//score/launch_manager",
        "//tests/utils/test_helper:fail_test",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":sandbox_options_non_root.json",
    env = {"NON_ROOT_ID": _NON_ROOT_ID},
    # Sanitizers cannot reserve their shadow memory under the RLIMIT_AS configured for the processes.
    target_compatible_with = [
        "@platforms//os:linux",
        "@score_cpp_policies//sanitizers/constraints:no_asan",
        "@score_cpp_policies//sanitizers/constraints:no_tsan",
    ],
)
