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

def integration_test(name, srcs, test_binaries, args = [], deps = [], data = [], env = {}, **kwargs):
    """Creates an integration test with test binaries available, also adds all
    the required dependencies.

    Args:
        test_binaries: Label of the `package_test_binaries` target
    """

    merged_srcs = srcs + ["//tests/utils/fixtures:conf"]
    merged_deps = deps + ["//tests/utils/fixtures"]
    merged_data = data + [test_binaries, "//tests/utils/environments:test_environment"]
    merged_args = args + select({
        "//config:host": ["--image-path=native"],
        "//config:x86_64-qnx": ["--image-path=$(location //tests/utils/environments:test_environment)"],
        "//conditions:default": ["--image-path=native"],
    })

    merged_env = dict(env)
    merged_env["SCORE_TEST_BINARY_PATH"] = "$(locations {})".format(test_binaries)
    merged_env["SCORE_TEST_REMOTE_DIRECTORY"] = "/opt/score/tests/{test_name}".format(test_name = name)

    # forward everything to score_py_pytest
    score_py_pytest(
        name = name,
        srcs = merged_srcs,
        deps = merged_deps,
        data = merged_data,
        env = merged_env,
        args = merged_args,
        **kwargs
    )

def _package_test_binaries_impl(ctx):
    """Packages test binaries into a given structure.
    """
    output_files = []

    for target, relative_location in ctx.attr.binaries.items():
        # it's possible a target is composed of multiple files so link all
        for file in target.files.to_list():
            output_file = ctx.actions.declare_file("opt/score/tests/{test_name}/{relative_location}/{proc}".format(
                test_name = ctx.attr.test_name,
                relative_location = relative_location,
                proc = file.basename,
            ))
            output_files.append(output_file)

            ctx.actions.symlink(
                output = output_file,
                target_file = file,
            )

    return [DefaultInfo(files = depset(output_files))]

package_test_binaries = rule(
    doc =
        """Packages binaries into a given structure.
    The binaries are symlinked.

    @details
    The file structure of the pacakge will be:
    `<current build dir>/opt/score/<test name>/<given path>`

    """,
    implementation = _package_test_binaries_impl,
    attrs = {
        "test_name": attr.string(
            mandatory = True,
            doc = "Name of the test that the binaries will belong to",
        ),
        "binaries": attr.label_keyed_string_dict(
            mandatory = True,
            allow_files = True,
            doc = "Dictionary mapping targets (need to be files) to a location",
        ),
    },
)
