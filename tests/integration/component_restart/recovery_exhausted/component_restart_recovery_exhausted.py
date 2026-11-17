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
def test_component_restart_recovery_exhausted(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the Launch Manager switches to the fallback Run Target when a
    component restart recovery action exceeds its retry budget.

    The Run Target Startup contains two components configured with the restart recovery action:
      - crashing_hanging: its restart never reports running, so its
        recovery is still in progress when the other recovery fails.
      - crashing_exhausted: activation_retry_attempts = 1, but it fails 2 restart attempts, so
        its recovery fails.
    The control client kills crashing_hanging, waits until its restart has started, and then
    kills crashing_exhausted.

    Expected Behaviour:
      - Run Target "Startup" stays active while the recoveries are in progress.
      - crashing_exhausted is started exactly activation_retry_attempts + 1 = 2 times during its
        recovery, then the Launch Manager activates the fallback Run Target.
      - The restart of crashing_hanging is stopped and not retried.

    Neither crashing component ever restarts successfully, so only the control client writes a report.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=[
            "-c",
            str(remote_test_dir / "etc/component_restart_recovery_exhausted.bin"),
        ],
        cwd=str(remote_test_dir),
    )

    assert_test_results({"control_client_test_driver.xml"})
