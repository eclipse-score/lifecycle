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
        "comp_req__launch_man__rt_activate_tout",
    ],
    partially_verifies=[
        "comp_req__launch_man__ra_rt_switch_rt",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_run_target_transition_timeout(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the Launch Manager fails a Run Target activation that
    takes longer than the transition timeout configured for that Run Target.

    "run_target_slow_activation" is configured with a transition timeout of 1000 ms
    and depends on slow_starting_process, which waits 3000 ms before reporting
    running. That process is given a ready_timeout_ms of 10000 ms, so the Run
    Target's transition timeout is reached first and the component's own ready
    timeout is never involved.

    Expected Behaviour: the activation of "run_target_slow_activation" is never
    reported as successful. Instead the configured recovery action is taken, so the
    control daemon receives an activation of "fallback_run_target" with the source
    kRecoveryAction, and the verification component that is part of that fallback
    touches the `fallback_reached` file, showing the fallback was really started.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/run_target_transition_timeout.bin")],
        cwd=str(remote_test_dir),
    )

    # The managed processes perform no verification of their own, the control daemon
    # asserts the outcome.
    assert_test_results({"control_client_test_driver.xml"})
