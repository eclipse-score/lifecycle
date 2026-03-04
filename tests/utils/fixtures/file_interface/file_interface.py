from abc import ABC, abstractmethod
from pathlib import Path
from typing import Generator


class FileInterface(ABC):
    """ """

    @abstractmethod
    def download(self, remote_path: Path, local_path: Path) -> None:
        """Download a file from the remote to a local path

        Args:
            remote_path: Path on the remote target.
            local_path: Local path to download to.
        """
        raise NotImplementedError()

    @abstractmethod
    def upload(self, local_path: Path, remote_path: Path) -> None:
        """Upload a file from the local path to the remote

        Args:
            local_path: Local path to upload the file from.
            remote_path: Path on the remote target.
        """
        raise NotImplementedError()

    @abstractmethod
    def walk_files(self, remote_path: Path) -> Generator[Path, None, None]:
        """Walk the given directory and yield all files.

        Args:
            remote_path: Remote path to walk.

        Returns:
            A generator yielding the paths to the file.
        """
        raise NotImplementedError()
