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

cc_library(
    name = "lifecycle_client_interface",
    hdrs = [
        "src/application.h",
        "src/lifecycle_client.h",
        "src/runapplication.h",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/launch_manager:error",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "lifecycle_client",
    srcs = [
        "src/aasapplicationcontainer.cpp",
        "src/application.cpp",
        "src/applicationcontext.cpp",
        "src/lifecycleclient.cpp",
        "src/lifecyclemanager.cpp",
        "src/runapplication.cpp",
    ],
    hdrs = [
        "src/aasapplicationcontainer.h",
        "src/application.h",
        "src/applicationcontext.h",
        "src/lifecyclemanager.h",
        "src/runapplication.h",
    ],
    copts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-pthread"],
    }),
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    linkopts = select({
        "@platforms//os:qnx": [],
        "@platforms//os:linux": ["-lpthread"],
    }),
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":lifecycle_client_interface",
        "//score/launch_manager/lifecycle_client/src/details:lifecycle_client_impl",
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
        "src/applicationcontext.cpp",
    ],
    hdrs = [
        "src/applicationcontext.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//visibility:public"],
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
        "src/application.cpp",
    ],
    hdrs = [
        "src/application.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//visibility:public"],
    deps = [
        ":applicationcontext",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "aasapplicationcontainer",
    srcs = [
        "src/aasapplicationcontainer.cpp",
    ],
    hdrs = [
        "src/aasapplicationcontainer.h",
        "src/application.h",
        "src/lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//visibility:public"],
    deps = [
        ":applicationcontext",
    ],
)

cc_library(
    name = "lifecyclemanager",
    srcs = [
        "src/lifecyclemanager.cpp",
    ],
    hdrs = [
        "src/application.h",
        "src/lifecyclemanager.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//visibility:public"],
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
            "src/runapplication.cpp",
        ],
        hdrs = [
            "src/application.h",
            "src/lifecyclemanager.h",
            "src/runapplication.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        include_prefix = "score/mw/lifecycle/lifecycle_client",
        strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
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
        "src/applicationcontextmock.cpp",
    ],
    hdrs = [
        "src/applicationcontext.h",
        "src/applicationcontextmock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//visibility:public"],
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
        "src/application.cpp",
    ],
    hdrs = [
        "src/application.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    deps = [
        ":applicationcontext_mock",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "lifecycle_mock",
    testonly = True,
    srcs = [
        "src/lifecyclemanagermock.cpp",
    ],
    hdrs = [
        "src/lifecyclemanager.h",
        "src/lifecyclemanagermock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//visibility:public"],
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
        "src/*.cpp",
        "src/*.h",
    ]),
)
