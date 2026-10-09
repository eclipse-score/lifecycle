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
    test_type="interface-test",
    derivation_technique="error-guessing",
)
def test_explicit_fallback_request(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that an explicit State Manager request for the fallback Run Target is reported with
    activation source kStateManagerRequest, not kRecoveryAction.

    The control client activates "fallback_run_target" by name although no process failed, then switches back to Startup.
    Expected Behaviour: Both activations are reported as kStateManagerRequest.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/explicit_fallback_request.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"control_client_test_driver.xml"})
