from .control_interface import ControlInterface
from .linux_interface import LinuxControl
from .ssh import SshInterface
from itf.core.com.ssh import Ssh
import pytest
        
@pytest.fixture
def control_interface(target, request) -> ControlInterface:

    # if no image provided then run natively
    if not request.config.getoption("--image-path"):
        yield LinuxInterface
    else:
        with Ssh("192.168.100.10") as ssh: 
            yield SshInterface(ssh)

