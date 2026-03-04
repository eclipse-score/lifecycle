from tests.utils.fixtures.control_interface.control_interface import ControlInterface
from tests.utils.fixtures.control_interface.linux_interface import LinuxControl
from tests.utils.fixtures.control_interface.ssh_interface import SshInterface
from tests.utils.fixtures.target.fixture import target
from itf.core.com.ssh import Ssh
import pytest


@pytest.fixture
def control_interface(target, request) -> ControlInterface:
    # if no image provided then run natively
    if request.config.getoption("--image-path") == "native":
        yield LinuxControl()

    else:
        with Ssh("192.168.100.10") as ssh:
            yield SshInterface(ssh)
