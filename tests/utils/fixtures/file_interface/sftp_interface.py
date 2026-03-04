from tests.utils.fixtures.file_interface.file_interface import FileInterface
from itf.core.com.sftp import Sftp
from pathlib import Path
from typing import Generator
import logging

logger = logging.getLogger(__name__)


class SftpInterface(FileInterface):
    def __init__(self, sftp: Sftp):
        self.__sftp = sftp
        pass

    def download(self, remote_path: Path, local_path: Path):
        logger.debug(f"Download: {remote_path} to {local_path}")
        return self.__sftp.download(str(remote_path), str(local_path))

    def upload(self, local_path: Path, remote_path: Path):
        logger.debug(f"Upload: {local_path} to {remote_path}")
        return self.__sftp.upload(str(local_path), str(remote_path))

    def walk_files(self, remote_path: Path) -> Generator[Path, None, None]:
        for path, files in self.__sftp.walk(str(remote_path)):
            for file in files:
                yield Path(path) / file
