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
    fully_verifies=["comp_req__launch_man__queue_rt_selection"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_activation_queue(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies activation queue behavior including:
    - Multiple items can be queued until max capacity (10) is reached
    - Requests are rejected when the queue is full
    - Queue behaves in FIFO (First-In-First-Out) order

    Expected Behaviour:
    - Block the activation queue by starting a gated activation
    - Queue 10 non-forced run target activations (filling the queue)
    - Attempt to queue an 11th activation (should be rejected - queue full)
    - Release the gated activation
    - Verify all 10 queued activations occur in FIFO order
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"control_client_test_driver.xml", "gated.xml"})
