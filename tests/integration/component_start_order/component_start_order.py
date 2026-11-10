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
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import assert_test_results
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__dep_based_startup_order",
    ],
    partially_verifies=[
        "feat_req__lifecycle__launch_support",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_component_start_order(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the launch manager starts processes in the order
    given by their dependencies.

    The config is such that:

    run_target_a(component_a -> component_b) -> run_target_b(component_c)

    Should observer:
    1. activate run_target_a
       component_a starts then
       component_b starts
    2. activate run_target_b
       component_c starts
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/component_start_order.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "start_order_process_a.xml",
            "start_order_process_b.xml",
            "start_order_process_c.xml",
            "control_client_test_driver.xml",
        }
    )
