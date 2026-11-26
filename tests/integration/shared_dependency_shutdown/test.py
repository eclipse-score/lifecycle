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

from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__stop_process_dependents",
    ],
    partially_verifies=[
        "feat_req__lifecycle__termination_dependency",
        "comp_req__launch_man__launch_parallel",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_shared_dependency_shutdown(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that a component with more than one dependent is only stopped once
    *every* dependent has fully terminated, not once a dependent has merely begun its
    graceful shutdown.

    The config is a diamond:

        run_target_diamond -> component_fast -> component_shared
                           -> component_slow -> component_shared

    component_fast and component_slow both run the dependent_process binary (parameterized by
    --stop-delay-ms) and are stopped in parallel. Both keep running after SIGTERM to model
    graceful shutdown work; component_fast exits after 300 ms, component_slow after 1500 ms.
    Each increments a shared stop counter immediately before exiting. component_shared runs the
    dedicated component_shared_process binary, which checks that counter in its own SIGTERM
    handler.

    Should observe:
    1. activate run_target_diamond
       component_shared starts, then component_fast and component_slow start
    2. activate Startup
       component_fast and component_slow are sent SIGTERM in parallel
       component_fast exits (stop counter 1), component_slow is still shutting down
       component_shared is sent SIGTERM only after component_slow has exited, i.e. with
       the stop counter at 2

    component_shared records a violation if its SIGTERM arrives while the stop counter is
    below 2. The test fails if any violation was recorded.

    See shared_dependency_shutdown.puml for a sequence diagram of both the required and
    the currently observed behaviour.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "component_shared_process.xml",
            "dependent_process_fast.xml",
            "dependent_process_slow.xml",
            "control_client_test_driver.xml",
        }
    )
