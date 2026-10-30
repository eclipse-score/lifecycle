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
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[],
    partially_verifies=[
        "comp_req__launch_man__launcher_exit_shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_lm_shutdown_with_control_requests(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the Launch Manager handles LmControl requests that arrive
    while it is shutting down without hanging or crashing.

    lm_control_requester keeps sending GetActiveRunTarget and ActivateRunTarget requests.
    The control client switches to the "Off" Run Target.
    We then delibrately hang in component_a, while terminating.
    This forces the test to send a SIGTERM to the launch manager.
    The lm_control_requester outlives the control client, so requests keep arriving
    until the launch manager has destroyed the ProcessGroupManager.

    Expected Behaviour: The launch manager shuts down and exits cleanly.
    """

    lm = target.execute_async(
        str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )
    requester = target.execute_async(
        str(remote_test_dir / "lm_control_requester"),
        args=[],
        cwd=str(remote_test_dir),
    )

    assert lm.wait(15) == 0
    assert requester.wait(30) == 0

    assert_test_results({"control_client_test_driver.xml", "component_a.xml"})
