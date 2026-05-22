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
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "lifecycle_client_interface",
    hdrs = ["lifecycle_client.h"],
    include_prefix = "score/mw/lifecycle",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/mw/lifecycle:error",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "lifecycle_client",
    srcs = [
        "aasapplicationcontainer.cpp",
        "application.cpp",
        "applicationcontext.cpp",
        "lifecycleclient.cpp",
        "lifecyclemanager.cpp",
        "runapplication.cpp",
    ],
    hdrs = [
        "aasapplicationcontainer.h",
        "application.h",
        "applicationcontext.h",
        "lifecyclemanager.h",
        "runapplication.h",
    ],
    copts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-pthread"],
    }),
    include_prefix = "score/mw/lifecycle",
    linkopts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-lpthread"],
    }),
    strip_include_prefix = "/score/mw/lifecycle/lifecycle_client",
    visibility = ["//visibility:public"],
    deps = [
        ":lifecycle_client_interface",
        "//score/mw/lifecycle/lifecycle_client/details:lifecycle_client_impl",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/memory:string_literal",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "applicationcontext",
    srcs = [
        "applicationcontext.cpp",
    ],
    hdrs = [
        "applicationcontext.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FUSA"],
    deps = [
        "@score_baselibs//score/memory:string_literal",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "application",
    srcs = [
        "application.cpp",
    ],
    hdrs = [
        "application.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FUSA"],
    deps = [
        ":applicationcontext",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "aasapplicationcontainer",
    srcs = [
        "aasapplicationcontainer.cpp",
    ],
    hdrs = [
        "aasapplicationcontainer.h",
        "application.h",
        "lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":applicationcontext",
    ],
)

cc_library(
    name = "lifecyclemanager",
    srcs = [
        "lifecyclemanager.cpp",
    ],
    hdrs = [
        "application.h",
        "lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FUSA"],
    deps = [
        ":application",
        ":lifecycle_client",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

[
    cc_library(
        name = name,
        srcs = [
            "runapplication.cpp",
        ],
        hdrs = [
            "application.h",
            "lifecyclemanager.h",
            "runapplication.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        tags = ["FUSA"],
        deps = [
            ":applicationcontext",
        ] + dependencies,
    )
    for name, dependencies in [
        (
            "lifecycle",
            [],
        ),
        (
            "lifecycle_non_adaptive",
            [],
        ),
    ]
]

cc_library(
    name = "applicationcontext_mock",
    testonly = True,
    srcs = [
        "applicationcontextmock.cpp",
    ],
    hdrs = [
        "applicationcontext.h",
        "applicationcontextmock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "@googletest//:gtest",
        "@score_baselibs//score/memory:string_literal",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "application_mock",
    testonly = True,
    srcs = [
        "application.cpp",
    ],
    hdrs = [
        "application.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":applicationcontext_mock",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "lifecycle_mock",
    testonly = True,
    srcs = [
        "lifecyclemanagermock.cpp",
    ],
    hdrs = [
        "lifecyclemanager.h",
        "lifecyclemanagermock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":application_mock",
        "@googletest//:gtest",
        "@score_baselibs//score/os/utils:signal",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
    ],
)

filegroup(
    name = "all_sources",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
)
