from tests.utils.fixtures.target import target
from tests.utils.fixtures.control_interface import ControlInterface
from itf.core.com.ssh import execute_command, Ssh
from typing import Tuple
from pathlib import Path
import pytest
import logging


logger = logging.getLogger(__name__)

class SshInterface(ControlInterface):
    """
    """

    def __init__(self, ssh: Ssh):
        self.__ssh = ssh

    def exec_command_blocking(self,
        *args: str, timeout=1, **env: str
    ) -> Tuple[int, str, str]:
        stdin, stdout, stderr = self.__ssh.exec_command(' '.join(args))

        ret_code = stdout.channel.recv_exit_status()

        return ret_code, stdout.read().decode('utf-8'), stderr.read().decode('utf-8')


    def run_until_file_deployed(self,
        *args,
        timeout=1,
        file_path=Path("tests/integration/test_end"),
        poll_interval=0.05,
        **env,
    ) -> Tuple[int, str, str]:
        pass

@pytest.fixture
def ssh(target) -> SshInterface:

    logger.info("Starting SSH Connection")
    with Ssh("192.168.100.10") as ssh:

        yield SshInterface(ssh)

        logger.info("Closing SSH Connection")


