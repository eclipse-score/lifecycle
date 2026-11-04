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
"""Build-variant machinery for the launch_manager daemon binary.

`debug_asserts_binary` rebuilds an existing cc_binary - and its WHOLE
dependency tree - with SCORE_LANGUAGE_FUTURECPP_ASSERT_LEVEL_DEBUG, via a
configuration transition on --copt.

A plain `defines = [...]` on a second cc_binary is not enough for this:
`defines` propagates to targets that depend on the binary, not to its
dependencies, so only the binary's own srcs (main.cpp) would see the macro
while all daemon library translation units kept DBG-level asserts compiled
out. The transition instead changes the build configuration for everything
below the binary, which is what the integration tests need.
"""

_DEBUG_ASSERTS_DEFINE = "-DSCORE_LANGUAGE_FUTURECPP_ASSERT_LEVEL_DEBUG"

def _enable_debug_asserts_impl(settings, _attr):
    copts = settings["//command_line_option:copt"]
    if _DEBUG_ASSERTS_DEFINE in copts:
        return {"//command_line_option:copt": copts}
    return {"//command_line_option:copt": copts + [_DEBUG_ASSERTS_DEFINE]}

_enable_debug_asserts = transition(
    implementation = _enable_debug_asserts_impl,
    inputs = ["//command_line_option:copt"],
    outputs = ["//command_line_option:copt"],
)

def _debug_asserts_binary_impl(ctx):
    target = ctx.attr.binary
    if type(target) == "list":
        target = target[0]

    executable = target[DefaultInfo].files_to_run.executable

    # Keeps the production basename (in a subdirectory) so test packaging
    # needs no renames.
    out = ctx.actions.declare_file(ctx.attr.out)
    ctx.actions.symlink(output = out, target_file = executable, is_executable = True)

    runfiles = ctx.runfiles(files = [out]).merge(target[DefaultInfo].default_runfiles)
    return [DefaultInfo(executable = out, runfiles = runfiles)]

debug_asserts_binary = rule(
    implementation = _debug_asserts_binary_impl,
    doc = "An existing cc_binary rebuilt with DBG-level asserts enabled in every translation unit.",
    executable = True,
    attrs = {
        "binary": attr.label(
            doc = "The production binary to rebuild with debug asserts.",
            cfg = _enable_debug_asserts,
            executable = True,
            mandatory = True,
        ),
        "out": attr.string(
            doc = "Output path for the variant; keep the production binary's basename.",
            mandatory = True,
        ),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
