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
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//tests/utils/bazel:integration.bzl", "integration_test")

# RLIMIT_FSIZE is not supported on QNX 8, so it is removed from the config.
# See https://www.qnx.com/developers/docs/8.0/com.qnx.doc.neutrino.utilities/topic/k/ksh.html#ksh__ulimit
# ulimit -f: "QNX OS currently doesn't support this limit."
_QNX_UNSUPPORTED_RLIMITS = {
    '                    "--rlimit-fsize=104857600,unlimited",\n': "",
    '                        "RLIMIT_FSIZE": {\n' +
    '                            "soft": 104857600,\n' +
    '                            "hard": "unlimited"\n' +
    "                        },\n": "",
}

expand_template(
    name = "sandbox_options_config",
    out = "sandbox_options.json",
    substitutions = select({
        "@platforms//os:qnx": _QNX_UNSUPPORTED_RLIMITS,
        "//conditions:default": {},
    }),
    template = "sandbox_options.in.json",
)

# Three binaries are built from the same source. Each verifies the sandbox options passed to it on
# the command line (see sandbox_options.in.json), and derives its GTest XML result file name from its
# own executable name so the results do not collide.
[
    cc_binary(
        name = "sandbox_options_process_{}".format(suffix),
        srcs = [
            "sandbox_options_process.cpp",
            "verify_sandbox.hpp",
        ],
        deps = [
            "//score/launch_manager:control_cc",
            "//score/launch_manager:lifecycle_cc",
            "//tests/utils/test_helper",
            "@googletest//:gtest_main",
        ],
    )
    for suffix in ("a", "b", "c")
]

integration_test(
    name = "sandbox_options",
    timeout = "short",
    srcs = ["sandbox_options.py"],
    binaries = [
        ":sandbox_options_process_a",
        ":sandbox_options_process_b",
        ":sandbox_options_process_c",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":sandbox_options.json",
)
