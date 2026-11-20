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
    fully_verifies=[],
    partially_verifies=[
        "feat_req__lifecycle__monitor_processes",
        "comp_req__launch_man__failure_detect",
    ],
    test_type="interface-test",
    derivation_technique="design-analysis",
)
def test_component_restart_recovery_succeeds(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies the component restart recovery action when several components
    terminate unexpectedly at the same time.

    The Run Target Startup contains three components configured with the restart recovery
    action (crashing_a, crashing_b, crashing_c) and a component (dependent) that depends on
    crashing_a and crashing_b. Once Startup is active, the control client kills all three
    crashing components with SIGKILL at the same time. The crashing components are configured
    with activation_retry_attempts = 0, 1 and 2 respectively, and each fails as many restart
    attempts as it has retries, so every recovery needs its full retry budget to succeed.

    Expected Behaviour:
      - Each crashing component is restarted and reaches its Ready State again.
      - The dependent component is neither stopped nor restarted.
      - Startup stays the active Run Target; the fallback Run Target (fail_test) is not activated.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=[
            "-c",
            str(remote_test_dir / "etc/component_restart_recovery_succeeds.bin"),
        ],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "control_client_test_driver.xml",
            "dependent_process.xml",
            "crashing_a.xml",
            "crashing_b.xml",
            "crashing_c.xml",
        }
    )
