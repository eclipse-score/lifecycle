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

#TODO: Review if we need both targets lifecycle_client_interface <--> lifecycle_client
cc_library(
    name = "lifecycle_client_interface",
    hdrs = [
        "src/application.h",
        "src/lifecycle_client.h",
        "src/runapplication.h",
    ],
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        "//score/launch_manager:error",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/result",
    ],
)

# TODO: Proposal from Nicolas, get rid of interface target and adjust include prefix to score/mw/lifecycle and adjust include path
cc_library(
    name = "lifecycle_client",
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
    visibility = ["//visibility:public"],
    deps = [
        ":aasapplicationcontainer",
        ":application",
        ":lifecyclemanager",
        ":runapplication",
        ":report_running",
        ":lifecycle_client_interface",
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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        ":application",
        ":report_running",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "runapplication",
    srcs = [
        "src/runapplication.cpp",
    ],
    hdrs = [
        "src/runapplication.h",
        "src/lifecyclemanager.h",
        "src/applicationcontext.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        ":application",
        ":report_running",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "report_running",
    srcs = ["src/report_running.cpp"],
    hdrs = ["src/report_running.h"],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    tags = ["FUSA"],
    deps = [
        ":lifecycleclient_internal",
    ],
)

cc_library(
    name = "lifecycleclient_internal",
    srcs = [
        "src/lifecycle_client.cpp",
    ],
    hdrs = [
        "src/lifecycle_client.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    tags = ["FUSA"],
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        "//score/launch_manager/lifecycle_client/src/details:lifecycle_client_impl",
    ],
)

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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
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
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        ":applicationcontext_mock",
        "@score_baselibs//score/os/utils:signal",
    ],
)

# TODO: Introduce public alias
cc_library(
    name = "lifecycle_mock",
    testonly = True,
    srcs = [
        "src/lifecyclemanagermock.cpp",
    ],
    hdrs = [
       # "src/lifecyclemanager.h",
        "src/lifecyclemanagermock.h",
       # "src/runapplication.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        ":application_mock",
        "@googletest//:gtest",
        "@score_baselibs//score/os/utils:signal",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
    ],
)

# TODO: Introduce public alias
cc_library(
    name = "report_running_mock",
    testonly = True,
    srcs = ["src/report_running_mock.cpp"],
    hdrs = [
        "src/report_running.h",
        "src/report_running_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    include_prefix = "score/mw/lifecycle/lifecycle_client",
    strip_include_prefix = "/score/launch_manager/lifecycle_client/src",
    visibility = ["//score/launch_manager/lifecycle_client:__subpackages__"],
    deps = [
        "@googletest//:gtest",
    ],
)

# TODO: Link against runapplication header
cc_test(
    name = "runapplication_UT",
    srcs = ["src/runapplication_UT.cpp"],
    deps = [
        ":lifecycle_mock",
        "@googletest//:gtest_main",
    ],
)
