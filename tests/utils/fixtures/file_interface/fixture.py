from tests.utils.fixtures.control_interface.fixture import control_interface
from tests.utils.fixtures.file_interface.sftp_interface import SftpInterface
from tests.utils.fixtures.file_interface.file_interface import FileInterface
from tests.utils.fixtures.file_interface.local_interface import LocalFile
from itf.core.com.sftp import Sftp
import pytest
import logging

logger = logging.getLogger(__name__)


@pytest.fixture
def file_interface(request, control_interface) -> FileInterface:
    if request.config.getoption("--image-path") == "native":
        yield LocalFile()
        return None

    logger.info("Starting SFTP Connection")
    with Sftp(control_interface.ssh, "not_needed") as sftp:
        yield SftpInterface(sftp)
        logger.info("Closing SFTP Connection")
