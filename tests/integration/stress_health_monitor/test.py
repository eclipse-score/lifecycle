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

# The health monitor runs under load for 10 seconds, which is close to the default timeout of run_test.
RUN_TIMEOUT_S = 30


@add_test_properties(
    fully_verifies=[],
    partially_verifies=[
        "comp_req__health_monitor__concurrency",
        "comp_req__health_monitor__cyclic_eval",
        "comp_req__health_monitor__alive_notify",
        "comp_req__health_monitor__hb_too_late",
        "comp_req__health_monitor__alive_withhold",
    ],
    test_type="interface-test",
    derivation_technique="explorative-testing",
)
def test_stress_health_monitor(
    target, setup_test, assert_test_results, remote_test_dir
):
    """
    Objective: Verifies that the health monitor supervises a large number of heartbeat monitors correctly.

    One supervised process registers 1024 heartbeat monitors and sends heartbeats to all of them from 16 threads
    for 10 seconds, then the heartbeats stop. The process also reports alive notifications to the launch manager.
    Expected Behaviour: While the heartbeats are sent, no heartbeat monitor reports an error, so the launch manager
    does not execute the recovery action. After the heartbeats stopped, the health monitor detects it and withholds
    the alive notifications, so the launch manager kills the process and activates the fallback Run Target.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/config.bin")],
        cwd=str(remote_test_dir),
        timeout=RUN_TIMEOUT_S,
    )

    assert_test_results(
        {"component_stress_health_monitor.xml", "control_client_test_driver.xml"}
    )
