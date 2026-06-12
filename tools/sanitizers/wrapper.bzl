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

"""Generates an executable script from a template, with the runfiles path of
another executable target baked in as a substitution."""

def _expand_executable_template_impl(ctx):
    out = ctx.actions.declare_file(ctx.attr.out)
    wrapper_label = ctx.attr.wrapper.label
    ctx.actions.expand_template(
        template = ctx.file.template,
        output = out,
        substitutions = {
            "%WRAPPER_RLOCATION%": "{}/{}/{}".format(
                wrapper_label.workspace_name,
                wrapper_label.package,
                wrapper_label.name,
            ),
        },
        is_executable = True,
    )
    return [DefaultInfo(files = depset([out]))]

expand_executable_template = rule(
    implementation = _expand_executable_template_impl,
    attrs = {
        "out": attr.string(mandatory = True),
        "template": attr.label(allow_single_file = True, mandatory = True),
        "wrapper": attr.label(mandatory = True),
    },
)
