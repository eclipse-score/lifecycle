import pytest
from pathlib import Path
import logging


logger = logging.getLogger(__name__)


@pytest.fixture
def setup_test(request, target):
    """Setsup the test by uploading all the binaries to the target"""

    bin_paths = request.config.getoption("--score-test-binary-path")
    bin_paths = [Path(p) for p in bin_paths.split(" ")]

    remote_dir = Path(request.config.getoption("--score-test-remote-directory"))
    anchor = remote_dir.parts[1]  # first path component, e.g. "opt" or "tmp"
    root_path_index = list(bin_paths[-1].parts).index(anchor)

    def upload_file(file):
        remote_path = Path("/", *file.parts[root_path_index:])
        res, _ = target.execute(f"mkdir -p {remote_path.parent}")
        assert res != 1, f"Couldn't create directory {remote_path.parent}"
        res, _ = target.execute(f"chmod 777 -R {remote_path.parent}")
        assert res != 1, f"Couldn't chmod directory {remote_path.parent}"
        target.upload(
            file.resolve(), remote_path
        )  # Need to resolve for https://github.com/eclipse-score/itf/pull/71

    for path in bin_paths:
        if path.is_dir():
            for file in path.rglob("*"):
                if file.is_file():
                    upload_file(file)
        else:
            assert path.is_file(), f"{path} is not a file or directory"
            upload_file(path)

    logger.info("Test case setup finished")
