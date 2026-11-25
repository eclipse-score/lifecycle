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
"""Generates the Launch Manager configuration for the stress_launch test.

The configuration contains the control client test driver (the State Manager) and the
Run Target "Stress", which depends on the driver and 1023 stress components, so that the
Launch Manager manages 1024 processes while it is active. Half of the stress components
are reporting and half are native processes. All of them run the same `counting_process`
binary, which posts to a named semaphore once it is started and then waits for SIGTERM.
"""

import argparse
import json

# Processes managed by the Launch Manager, including the driver.
# 1024 is the Launch Manager limit (ProcessLimits::kMaxProcesses).
PROCESSES = 1024

# Time budget for the Run Target transitions that start or stop all stress processes.
TRANSITION_TIMEOUT_MS = 120000
# Time budget for a single stress process to become ready while all others start too.
READY_TIMEOUT_MS = 60000


def stress_component(index: int, semaphore: str) -> tuple:
    """Returns the name and configuration of the stress component `index`."""
    arguments = ["--post-to-semaphore", semaphore, "--wait-for-termination"]
    if index % 2 == 0:
        return f"reporting_{index:04d}", {
            "component_properties": {
                "process_arguments": ["--report-running"] + arguments,
                "application_profile": {"application_type": "Reporting"},
            }
        }
    return f"native_{index:04d}", {
        "component_properties": {
            "process_arguments": arguments,
            "application_profile": {"application_type": "Native"},
        }
    }


def generate(name: str) -> dict:
    """Returns the configuration of the stress_launch test `name`."""
    semaphore = f"/{name}"
    driver = "control_client_test_driver"

    stress_components = dict(
        stress_component(index, semaphore) for index in range(PROCESSES - 1)
    )

    components = {
        driver: {
            "component_properties": {
                "binary_name": driver,
                "process_arguments": [
                    "--semaphore",
                    semaphore,
                    "--expected-count",
                    str(PROCESSES - 1),
                ],
                "application_profile": {
                    "application_type": "State_Manager",
                    "alive_supervision": {"min_indications": 0},
                },
            },
            "deployment_config": {
                "environmental_variables": {"PROCESSIDENTIFIER": driver}
            },
        },
        **stress_components,
    }

    run_targets = {
        "Startup": {"depends_on": [driver]},
        "Stress": {"depends_on": list(components)},
        "Off": {"depends_on": []},
    }

    return {
        "schema_version": 2,
        "defaults": {
            "deployment_config": {
                "bin_dir": f"/tmp/tests/{name}",
                "ready_timeout_ms": READY_TIMEOUT_MS,
                "shutdown_timeout_ms": 5000,
                "environmental_variables": {"LD_LIBRARY_PATH": "/opt/lib"},
                "sandbox": {
                    "uid": 0,
                    "gid": 0,
                    "scheduling_policy": "SCHED_OTHER",
                    "scheduling_priority": 0,
                },
            },
            "component_properties": {
                "binary_name": "counting_process",
                "application_profile": {
                    "application_type": "Native",
                    "is_self_terminating": False,
                },
                "ready_condition": {"process_state": "Running"},
            },
            "run_target": {"transition_timeout_ms": TRANSITION_TIMEOUT_MS},
        },
        "components": components,
        "run_targets": run_targets,
        "initial_run_target": "Startup",
        "fallback_run_target": {"depends_on": [driver]},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--name", required=True, help="Name of the test")
    parser.add_argument(
        "--output", required=True, help="Path of the generated configuration"
    )
    args = parser.parse_args()

    with open(args.output, "w", encoding="utf-8") as output:
        json.dump(generate(args.name), output, indent=4)
        output.write("\n")


if __name__ == "__main__":
    main()
