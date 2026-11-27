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
        "comp_req__launch_man__ra_rt_queue",
        "comp_req__launch_man__rt_queue",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_request_during_recovery(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies behavior of non-forced activation requests and queue preservation during recovery actions.

    Verifies: comp_req__launch_man__ra_rt_queue AND comp_req__launch_man__rt_queue
    - A component fails, triggering a recovery action to switch to fallback_run_target.
    - During the recovery action (while activation is in progress), a non-forced request
      for another run target is sent.
    - The request is queued (not rejected).
    - After the recovery action completes, the queued run target is activated.
    - This tests both queueing during recovery AND queue preservation after recovery.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
    )

    assert_test_results(
        {
            "control_client_test_driver.xml",
        }
    )
