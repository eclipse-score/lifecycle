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
    partially_verifies=[
        "comp_req__launch_man__conf_of_comp_ready_cond",
        "feat_req__lifecycle__conditional_startup",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_ready_condition_process_state(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that a component with a kTerminated process_state ready condition only
    reaches its ready state once the process has terminated.

    The initial run target contains a component that terminates after a delay.

    Expected Behaviour: The launch manager waits for the process to terminate or
    or the timeout to expire, whichever happens first.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/kterminated.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "control_client_test_driver.xml",
        }
    )
