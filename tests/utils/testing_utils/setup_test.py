import pytest
from os import environ, path
from pathlib import Path
import logging
from time import sleep
from typing import List


logger = logging.getLogger(__name__)


@pytest.fixture
def setup_tests(request, target):
    """ Setsup the test by uploading all the binaries to the target
    """

    bin_paths = request.config.getoption("--score-test-binary-path")
    bin_paths = [Path(p) for p in bin_paths.split(" ")]

    root_path_index = list(bin_paths[-1].parts).index("opt")

    for file in bin_paths:
        assert file.is_file(), f"{file} is not a file"
        remote_path = Path("/", *file.parts[root_path_index:])

        logger.info(target.execute(f"mkdir -p {remote_path.parent}"))
        logger.info(target.execute(f"chmod 777 -R {remote_path.parent}"))

        target.upload(file, remote_path)

    logger.info("Test case setup finished")
