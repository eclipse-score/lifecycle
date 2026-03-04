from tests.utils.fixtures.file_interface.file_interface import FileInterface
from pathlib import Path
from shutil import copy
from typing import Generator
import logging

logger = logging.getLogger(__name__)


class LocalFile(FileInterface):
    def download(self, remote_path: Path, local_path: Path):
        copy(remote_path, local_path)

    def upload(self, local_path: Path, remote_path: Path):
        logger.info(f"Making dir {remote_path.parent}")
        (remote_path.parent).mkdir(parents=True, exist_ok=True)
        if remote_path.is_file():
            return
        remote_path.symlink_to(local_path.resolve())

    def walk_files(self, remote_path: Path) -> Generator[Path, None, None]:
        for path, _, files in remote_path.walk():
            for file in files:
                yield path / file
