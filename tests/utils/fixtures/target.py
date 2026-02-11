import pytest
from typing import Optional
import logging
from time import sleep
from subprocess import Popen, PIPE, STDOUT
from pathlib import Path

from itf.core.qemu.qemu import Qemu
from itf.core.com.ssh import Ssh
from itf.core.utils.process.console import PipeConsole

logger = logging.getLogger(__name__)

@pytest.fixture
def target(request) -> Optional[Popen]:
    """Returns the target instance
    """

    logger.info("Starting Target")
    subprocess_params = {
        "stdin": PIPE,
        "stdout": PIPE,
        "stderr": STDOUT,
    }

    # if no image provided then run natively
    if not request.config.getoption("--image-path"):
        yield None

    image_location = Path(request.config.getoption("--image-path"))
    if not image_location.is_file():
        raise RuntimeError("No image")
    qemu = Qemu(str(image_location),
                host_first_network_device_ip_address="192.168.100.1")

    proc = qemu.start(subprocess_params)
    # console = PipeConsole("QEMU", proc)
    sleep(2)
    yield qemu
    logger.info("Closing Target")
    qemu.stop()
