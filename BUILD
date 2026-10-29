# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_docs_as_code//:bzl/bundle_rules.bzl", "bundle_source_files")
load("@score_docs_as_code//:docs.bzl", "docs", "docs_bundle")
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker", "setup_starpls")
load("@score_tooling//third_party/format:macros.bzl", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")

# In order to update the requirements, change the `requirements.in` file and run:
# `bazel run //:requirements.update`.
# This will update the `requirements_lock.txt` file.
# To upgrade all dependencies to their latest versions, run:
# `bazel run //:requirements.update -- --upgrade`.
compile_pip_requirements(
    name = "requirements",
    src = "requirements.in",
    data = [
        "//scripts/config_mapping:pip_requirements",
    ],
    extra_args = [
        "--no-annotate",
    ],
    requirements_txt = "requirements_lock.txt",
    tags = [
        "manual",
    ],
)

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        "BUILD",
        "MODULE.bazel",
        "docs",
        "docs_root_only",
        "examples",
        "externals",
        "quality",
        "score",
        "scripts",
        "tests",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    exclusion = "//:cr_checker_exclusion",
    extensions = [
        "bazel",
        "BUILD",
        "bzl",
        "c",
        "cpp",
        "h",
        "hpp",
        "ini",
        "py",
        "rs",
        "rst",
        "sh",
        "yaml",
        "yml",
    ],
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Needed for Dash tool to check python dependency licenses.
filegroup(
    name = "cargo_lock",
    srcs = [
        "Cargo.lock",
    ],
    visibility = ["//visibility:public"],
)

dash_license_checker(
    src = "//:cargo_lock",
    file_type = "",  # let it auto-detect based on project_config
    project_config = PROJECT_CONFIG,
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets(languages = [
    "python",
    "rust",
    "starlark",
    "yaml",
    "cpp",
])

# Required for code coverage reporting.
# The coverage reporter locates the workspace root at runtime via //:MODULE.bazel.
exports_files(["MODULE.bazel"])

# Docs
_IS_ROOT_MODULE = repository_name() == "@"

bundle_source_files(
    name = "platform_docs_sources",
    bundle = "@score_platform//:docs_bundle",
)

genrule(
    name = "platform_glossary_rst",
    srcs = [":platform_docs_sources"],
    outs = ["platform_glossary/glossary.rst"],
    cmd = "cp $$(printf '%s\\n' $(SRCS) | grep '/docs/features/lifecycle/glossary[.]rst$$') $@",
)

docs_bundle(
    name = "platform_glossary",
    srcs = [":platform_glossary_rst"],
    entry_doc = "glossary",
)

docs(
    bundles = [
        {
            "bundle": "//score/launch_manager:docs",
            "mount_at": "components/launch_manager",
        },
        {
            "bundle": "//score/health_monitor:docs",
            "mount_at": "components/health_monitor",
        },
    ] + ([
        # Only for lifecycle's own doc build, not during integration in ref_int repository
        {
            # Glossary is located in score_platform; avoid duplication in ref_int
            "attach_to": "features/lifecycle/index",
            "bundle": ":platform_glossary",
            "mount_at": "features/lifecycle",
        },
        {
            # Needs the local Sphinx extension, which ref_int does not load
            "attach_to": "verification_report/module_verification_report",
            "bundle": "//docs_root_only:docs",
            "mount_at": "verification_report/feat_req_verification_state",
        },
    ] if _IS_ROOT_MODULE else []),
    data = ["docs/feat_req_verification_table.py"] if _IS_ROOT_MODULE else [],  # Local Sphinx extension loaded by docs/conf.py
    external_needs = [
        "@score_platform//:needs_json",  # This allows linking to feature requirements.
        "@score_process_description//:needs_json",  # This allows linking to requirements (wp__requirements_comp, etc.) from the process_description repository.
    ],
    project = "Lifecycle and Health Management",
    project_url = "https://eclipse-score.github.io/lifecycle/",
    source_dir = "docs",
)
