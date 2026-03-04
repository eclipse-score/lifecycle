import pytest
from typing import Optional, Any
import logging
from time import sleep
from subprocess import PIPE, STDOUT
from pathlib import Path

from itf.core.qemu.qemu import Qemu

logger = logging.getLogger(__name__)


@pytest.fixture
def target(request) -> Optional[Any]:
    """Returns the target instance"""

    # if no image provided then run natively
    if request.config.getoption("--image-path") == "native":
        yield None
        return None

    logger.info("Starting Target")
    subprocess_params = {
        "stdin": PIPE,
        "stdout": PIPE,
        "stderr": STDOUT,
    }

    image_location = Path(request.config.getoption("--image-path"))

    if not image_location.is_file():
        raise RuntimeError(f"No image found under {image_location}")

    qemu = Qemu(
        str(image_location), host_first_network_device_ip_address="192.168.100.1"
    )

    _ = qemu.start(subprocess_params)
    sleep(2)  # wait for system to boot
    yield qemu
    logger.info("Closing Target")
    qemu.stop()
