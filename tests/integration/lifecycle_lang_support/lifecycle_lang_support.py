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
from attribute_plugin import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__lifecycle_lang_support",
        "feat_req__lifecycle__prog_lang",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_lifecycle_lang_support(target, setup_test, remote_test_dir):
    """
    Objective: Verifies that the launch manager supports C++, Rust and C
    applications reporting their readiness via the Lifecycle Interface.

    All component are configured with the ready condition "Running".
    The verification component depends on all three, so it is only started once
    every one of them has reported running, and then terminates the test.
    Expected Behaviour: The Startup Run Target is activated and the verification component runs.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/lifecycle_lang_support.bin")],
        cwd=str(remote_test_dir),
    )
