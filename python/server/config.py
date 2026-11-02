import os
import sys
from pathlib import Path


def user_data_dir() -> Path:
    """Resolve independently of the working directory; overrides must be absolute."""
    override = os.environ.get("MUSICPLAYER_DATA_DIR")
    if override:
        path = Path(override).expanduser()
        if not path.is_absolute():
            raise ValueError("MUSICPLAYER_DATA_DIR must be an absolute path")
        return path
    if sys.platform == "win32":
        base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "AppData" / "Local"))
    elif sys.platform == "darwin":
        base = Path.home() / "Library" / "Application Support"
    else:
        base = Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local" / "share"))
        if not base.is_absolute():
            base = Path.home() / ".local" / "share"
    return base / "MusicPlayer"


from dataclasses import dataclass


@dataclass(frozen=True)
class ServerRuntimeConfig:
    request_timeout_sec: float = 12.0
    stream_timeout_sec: float = 20.0
    stream_chunk_size: int = 64 * 1024
    default_host: str = "127.0.0.1"
    default_port: int = 8000
