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
cc_library(
    name = "return_types",
    hdrs = ["return_types.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "semaphore",
    srcs = ["details/posix/semaphore.cpp"],
    hdrs = ["semaphore.hpp"],
    visibility = ["//score:__subpackages__"],
    deps = [":return_types"],
)

cc_library(
    name = "sys_exit",
    srcs = ["details/posix/sys_exit.cpp"],
    hdrs = ["sys_exit.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "num_cores",
    srcs = select({
        "@platforms//os:linux": ["details/linux/num_cores.cpp"],
        "@platforms//os:qnx": ["details/qnx/num_cores.cpp"],
    }),
    hdrs = ["num_cores.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "security_policy",
    srcs = select({
        "@platforms//os:linux": ["details/linux/security_policy.cpp"],
        "@platforms//os:qnx": ["details/qnx/security_policy.cpp"],
    }),
    hdrs = ["security_policy.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "set_affinity",
    srcs = select({
        "@platforms//os:linux": ["details/linux/set_affinity.cpp"],
        "@platforms//os:qnx": ["details/qnx/set_affinity.cpp"],
    }),
    hdrs = ["set_affinity.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "set_groups",
    srcs = select({
        "@platforms//os:linux": ["details/linux/set_groups.cpp"],
        "@platforms//os:qnx": ["details/qnx/set_groups.cpp"],
    }),
    hdrs = ["set_groups.hpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "ipc_comms",
    hdrs = ["ipc_comms.hpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":semaphore",
        "//score/mw/launch_manager/common:log",
    ],
)

cc_library(
    name = "osal",
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ipc_comms",
        ":num_cores",
        ":return_types",
        ":security_policy",
        ":semaphore",
        ":set_affinity",
        ":set_groups",
        ":sys_exit",
    ],
)
