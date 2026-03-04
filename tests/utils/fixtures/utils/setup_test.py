import pytest
from os import environ, path
from pathlib import Path
from tests.utils.fixtures.file_interface.fixture import file_interface
import logging
from time import sleep
from typing import List


logger = logging.getLogger(__name__)


@pytest.fixture(scope="function")
def test_dir():
    return Path(environ["TEST_UNDECLARED_OUTPUTS_DIR"])


def remote_binary_paths() -> List[Path]:
    bin_paths = environ["SCORE_TEST_BINARY_PATH"]
    bin_paths = [Path(p) for p in bin_paths.split(" ")]
    root_path_index = list(bin_paths[-1].parts).index("opt")
    return [Path("/", *p.parts[root_path_index:]) for p in bin_paths]


@pytest.fixture(scope="function")
def download_test_results(file_interface, test_dir):
    remote_dir = Path(environ["SCORE_TEST_REMOTE_DIRECTORY"])

    def _():
        for file in file_interface.walk_files(remote_dir):
            if file.suffix == ".xml":
                local = test_dir / file.name
                logger.info(f"Downloading {file} to {local}")
                file_interface.download(file, local)

    return _


@pytest.fixture
def setup_tests(request, file_interface, control_interface):
    # upload all binaries required by test
    bin_paths = environ["SCORE_TEST_BINARY_PATH"]
    bin_paths = [Path(p) for p in bin_paths.split(" ")]

    root_path_index = list(bin_paths[-1].parts).index("opt")

    for file in bin_paths:
        assert file.is_file(), f"{file} is not a file"
        remote_path = Path("/", *file.parts[root_path_index:])
        file_interface.upload(file, remote_path)

        if request.config.getoption("--image-path") != "native":
            ret, _, stderr = control_interface.exec_command_blocking(
                f"chmod +x {str(remote_path)}"
            )
            assert ret == 0, f"Ret code: {ret}, {stderr}"

    logger.info("Test case setup finished")
