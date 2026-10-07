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
# AI Disclosure: Starlark generation logic was AI-generated with DeepSeek
# V4 Flash via Fabro. AI-generated portions are offered under CC0-1.0;
# copied configuration/profile data retains its original Apache-2.0 terms.
# Human review of this notice-adjusted revision is pending before merge.
# Assisted-by: DeepSeek V4 Flash (Fabro; recorded model label)
# SPDX-License-Identifier: Apache-2.0 AND CC0-1.0
# *******************************************************************************

"""Generated mw_com_config.json artifacts for the LmControl service.

Each supported profile emits the exact mw_com_configuration JSON document that
was previously committed as a checked-in source file. The profile-name map is
disclosure metadata only and is never emitted as a root key.
"""

load("@bazel_skylib//rules:write_file.bzl", "write_file")

_LM_CONTROL_SERVICE_TYPE = {
    "serviceTypeName": "/score/mw/lifecycle/LmControlService",
    "version": {
        "major": 1,
        "minor": 0,
    },
    "bindings": [
        {
            "binding": "SHM",
            "serviceId": 7101,
            "events": [
                {
                    "eventName": "ActivationResult",
                    "eventId": 1,
                },
            ],
            "methods": [
                {
                    "methodName": "ActivateRunTarget",
                    "methodId": 2,
                },
                {
                    "methodName": "GetActiveRunTarget",
                    "methodId": 3,
                },
            ],
        },
    ],
}

_SERVICE_TYPES = {
    "provider_test": [_LM_CONTROL_SERVICE_TYPE],
    "client_test": [_LM_CONTROL_SERVICE_TYPE],
    "integration": [_LM_CONTROL_SERVICE_TYPE],
}

_SERVICE_INSTANCES = {
    "provider_test": [
        {
            "instanceSpecifier": "LaunchManager/StateManager/Instance",
            "serviceTypeName": "/score/mw/lifecycle/LmControlService",
            "version": {
                "major": 1,
                "minor": 0,
            },
            "instances": [
                {
                    "instanceId": 1,
                    "asil-level": "QM",
                    "binding": "SHM",
                    "events": [
                        {
                            "eventName": "ActivationResult",
                            "numberOfSampleSlots": 8,
                            "maxSubscribers": 1,
                        },
                    ],
                    "methods": [
                        {
                            "methodName": "ActivateRunTarget",
                            "queueSize": 1,
                        },
                        {
                            "methodName": "GetActiveRunTarget",
                            "queueSize": 1,
                        },
                    ],
                },
            ],
        },
    ],
    "client_test": [
        {
            "instanceSpecifier": "StateManager/LaunchManager/Instance",
            "serviceTypeName": "/score/mw/lifecycle/LmControlService",
            "version": {
                "major": 1,
                "minor": 0,
            },
            "instances": [
                {
                    "instanceId": 1,
                    "asil-level": "B",
                    "binding": "SHM",
                    "permission-checks": "strict",
                    "allowedProvider": {
                        "B": [3020],
                    },
                    "events": [
                        {
                            "eventName": "ActivationResult",
                        },
                    ],
                    "methods": [
                        {
                            "methodName": "ActivateRunTarget",
                            "queueSize": 1,
                        },
                        {
                            "methodName": "GetActiveRunTarget",
                            "queueSize": 1,
                        },
                    ],
                },
            ],
        },
    ],
    "integration": [
        {
            "instanceSpecifier": "LaunchManager/StateManager/Instance",
            "serviceTypeName": "/score/mw/lifecycle/LmControlService",
            "version": {
                "major": 1,
                "minor": 0,
            },
            "instances": [
                {
                    "instanceId": 1,
                    "asil-level": "QM",
                    "binding": "SHM",
                    "events": [
                        {
                            "eventName": "ActivationResult",
                            "numberOfSampleSlots": 8,
                            "maxSubscribers": 1,
                        },
                    ],
                    "methods": [
                        {
                            "methodName": "ActivateRunTarget",
                            "queueSize": 1,
                        },
                        {
                            "methodName": "GetActiveRunTarget",
                            "queueSize": 1,
                        },
                    ],
                },
            ],
        },
        {
            "instanceSpecifier": "StateManager/LaunchManager/Instance",
            "serviceTypeName": "/score/mw/lifecycle/LmControlService",
            "version": {
                "major": 1,
                "minor": 0,
            },
            "instances": [
                {
                    "instanceId": 1,
                    "asil-level": "QM",
                    "binding": "SHM",
                    "events": [
                        {
                            "eventName": "ActivationResult",
                        },
                    ],
                    "methods": [
                        {
                            "methodName": "ActivateRunTarget",
                            "queueSize": 1,
                        },
                        {
                            "methodName": "GetActiveRunTarget",
                            "queueSize": 1,
                        },
                    ],
                },
            ],
        },
    ],
}

_GLOBAL = {
    "provider_test": {
        "asil-level": "QM",
    },
    "client_test": {
        "asil-level": "B",
        "queue-size": {
            "B-receiver": 10,
        },
    },
    "integration": {
        "asil-level": "QM",
    },
}

def _render(profile):
    return json.encode_indent(
        {
            "serviceTypes": _SERVICE_TYPES[profile],
            "serviceInstances": _SERVICE_INSTANCES[profile],
            "global": _GLOBAL[profile],
        },
        indent = "  ",
    )

def mw_com_config(name, profile, out, **kwargs):
    """Emit the mw_com_configuration JSON document for ``profile`` as ``out``."""
    write_file(
        name = name,
        out = out,
        content = [_render(profile)],
        **kwargs
    )
