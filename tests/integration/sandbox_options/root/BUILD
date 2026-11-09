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
    template = "//tests/integration/sandbox_options:sandbox_options.in.json",
)

integration_test(
    name = "sandbox_options",
    timeout = "short",
    srcs = ["sandbox_options.py"],
    binaries = [
        "//tests/integration/sandbox_options:sandbox_options_process_a",
        "//tests/integration/sandbox_options:sandbox_options_process_b",
        "//tests/integration/sandbox_options:sandbox_options_process_c",
        "//score/launch_manager",
        "//tests/utils/test_helper:verification_process",
    ],
    config = ":sandbox_options.json",
)
