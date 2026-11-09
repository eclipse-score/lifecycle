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
import logging
from tests.utils.testing_utils.setup_test import setup_test
from tests.utils.testing_utils.run_test import run_test
from tests.utils.testing_utils.test_results import assert_test_results
from tests.utils.testing_utils.realtime_docker import (  # noqa: F401
    docker_configuration,
    require_realtime_scheduling,
)
from attribute_plugin import add_test_properties

logger = logging.getLogger(__name__)


@add_test_properties(
    fully_verifies=[
        "comp_req__launch_man__uid_gid_support",
        "comp_req__launch_man__launch_priority_support",
        "comp_req__launch_man__scheduling_policy",
        "comp_req__launch_man__cwd_support",
        "comp_req__launch_man__supplementary_groups",
        "comp_req__launch_man__process_rlimit_support",
        "comp_req__launch_man__rc_os_state",
        "comp_req__launch_man__rt_comp_dep",
        "comp_req__launch_man__runmask_support",
    ],
    partially_verifies=[
        "feat_req__lifecycle__launch_support",
        "feat_req__lifecycle__sandbox_options",
        "feat_req__lifecycle__conditional_startup",
        "comp_req__launch_man__process_launch_args",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_sandbox_options(target, setup_test, assert_test_results, remote_test_dir):
    """
    Objective: Verifies the effectiveness of sandbox-options as gid, uid, supplementary groups, scheduling policy, and CPU affinity mask.

    The launch manager starts with an initial Run Target. The control daemon activates the "Running" Run Target (starting the managed process with the sandbox options applied), then transitions back to "Startup".
    Expected Behaviour: All Run Target transitions complete successfully and all processes report running.
    """

    run_test(
        target=target,
        binary_path=str(remote_test_dir / "launch_manager"),
        args=["-c", str(remote_test_dir / "etc/sandbox_options.bin")],
        cwd=str(remote_test_dir),
    )

    # sandbox_options_process_a is configured with a custom working directory ("working_dir": "/tmp"
    # in sandbox_options.in.json), so its XML result file is written there rather than into
    # remote_test_dir. sandbox_options_process_b has no working directory configured and therefore
    # writes into remote_test_dir. Search all relevant directories.
    assert_test_results(
        {
            "sandbox_options_process_a.xml",
            "sandbox_options_process_b.xml",
            "sandbox_options_process_c.xml",
        },
        additional_search_dirs=["/tmp/tests/sandbox_options", "/tmp"],
    )
