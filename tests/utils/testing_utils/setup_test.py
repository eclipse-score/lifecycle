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
import pytest
from pathlib import Path
import logging
from score.itf.core.target import Target


logger = logging.getLogger(__name__)


def setup(request: pytest.FixtureRequest, target: Target):
    # silence logs coming from docker which uses urllib3
    logging.getLogger("urllib3").setLevel(logging.WARNING)

    bin_path = Path(request.config.getoption("--score-test-binary-path"))
    remote_dir = Path(request.config.getoption("--score-test-remote-directory"))

    extract_dir = remote_dir.parent.parent  # e.g. /tmp
    remote_tar = extract_dir / bin_path.name

    res, _ = target.execute(f"mkdir -p {extract_dir}")
    assert res != 1, f"Couldn't create directory {extract_dir}"

    target.upload(
        bin_path.resolve(), remote_tar
    )  # Need to resolve for https://github.com/eclipse-score/itf/pull/71

    res, _ = target.execute(f"tar -xf {remote_tar} -C {extract_dir}")
    assert res == 0, f"Couldn't extract tar {remote_tar}"

    res, out = target.execute("ulimit -c unlimited")

    res, out = target.execute("cat /proc/sys/kernel/core_pattern")
    logger.info(f"core_pattern: {out}")

    logger.warning(f"ulimit res:\n{out.decode()}")
    if not res == 0:
        logger.warning(
            f"Failed to set the ulimit, core dumps might not be created:\n{out}"
        )
    res, out = target.execute("ulimit -c")
    logger.info(f"ulimit set:{out.decode()}")

    logger.info("Test case setup finished")


def download_core_dumps(target: Target, remote_test_dir: Path, test_output_dir: Path):
    """ """
    logger.info("Trying to download core dumps")

    # get core dumps
    res, stdout = target.execute(f"find {remote_test_dir} -name 'core*' -type f")
    if res != 0:
        logger.error("Find command errored")
        return

    core_files = stdout.decode().strip().splitlines()
    for remote_path in core_files:
        remote_path = remote_path.strip()
        if not remote_path:
            continue
        local_path = test_output_dir / Path(remote_path).name
        try:
            target.download(remote_path, str(local_path))
            logger.info(f"Downloaded core dump: {remote_path} -> {local_path}")
        except Exception as e:
            logger.warning(f"Failed to download core dump {remote_path}: {e}")


def teardown(target, remote_test_dir, test_output_dir):
    """Tears down the test case"""

    logger.info("Test case teardown started")
    download_core_dumps(target, remote_test_dir, test_output_dir)


@pytest.fixture(autouse=True, scope="function")
def setup_test(
    request: pytest.FixtureRequest,
    target: Target,
    remote_test_dir: Path,
    test_output_dir: Path,
):
    """Sets up and tears down the test"""

    setup(request, target)

    yield None

    teardown(target, remote_test_dir, test_output_dir)
