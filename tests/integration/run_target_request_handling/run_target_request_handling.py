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
    partially_verifies=[],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_run_target_request_handling(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Characterizes how the launch manager answers Run Target activation
    requests that do not lead to a plain, uninterrupted transition.

    The control client requests, in order: a Run Target that does not exist;
    run_target_a twice, the second time while the first activation is still in
    progress; run_target_a again once it is active; and, while run_target_b is
    being activated, Startup. gated_a and gated_b only report running once the
    control client releases them, which keeps those activations in progress for
    as long as the control client needs.

    Expected Behaviour: The unknown Run Target is rejected with
    kRunTargetDoesntExist, the repeated request during the activation with
    kInTransitionToSameState, and the request for the active Run Target with
    kAlreadyInState. The request for Startup is accepted and replaces the
    activation of run_target_b: the next activation reported is Startup, and
    run_target_b is never reported as activated.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/run_target_request_handling.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {"control_client_test_driver.xml", "gated_a.xml", "gated_b.xml"}
    )
