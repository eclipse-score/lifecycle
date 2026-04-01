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
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_lifecycle_pip//:requirements.bzl", "all_requirements")
load("//tests/utils/bazel:constants.bzl", "SCORE_TEST_INSTALL_PREFIX")

def integration_test(name, srcs, test_binaries, args = [], deps = [], data = [], install_prefix = SCORE_TEST_INSTALL_PREFIX, **kwargs):
    """Creates an integration test with test binaries available, also adds all
    the required dependencies.

    Args:
        test_binaries: Label of the `package_test_binaries` target
    """

    merged_data = data + [test_binaries] + select({
        "//config:host": [],
        "//conditions:default": ["//tests/utils/environments:test_environment"],
    })

    py_itf_test(
        name = name,
        srcs = srcs,
        deps = deps + all_requirements + ["@score_tooling//python_basics/score_pytest:attribute_plugin"],
        data = merged_data,
        args = args + [
            "--score-test-binary-path=$(locations {})".format(test_binaries),
            "--score-test-remote-directory={}/tests/{}".format(install_prefix, name),
        ] + select({
            "//config:x86_64-linux": [
                "--docker-image-bootstrap=$(location //tests/utils/environments:test_environment)",
                "--docker-image=score_itf_examples:latest",
            ],
            "//config:host": [
                "--local-dir=/tmp/score_itf_host/{}".format(name),
            ],
            "//conditions:default": [],
        }),
        plugins = ["//tests/utils/plugins:integration_plugin"] + select({
            "//config:x86_64-linux": ["@score_itf//score/itf/plugins:docker_plugin"],
            "//config:x86_64-qnx": ["@score_itf//score/itf/plugins/qemu"],
            "//config:host": ["//tests/utils/plugins:localhost_plugin"],
            "//conditions:default": [],
        }),
        **kwargs
    )
