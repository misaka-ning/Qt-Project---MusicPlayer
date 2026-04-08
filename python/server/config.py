from dataclasses import dataclass


@dataclass(frozen=True)
class ServerRuntimeConfig:
    request_timeout_sec: float = 12.0
    stream_timeout_sec: float = 20.0
    stream_chunk_size: int = 64 * 1024
    default_host: str = "127.0.0.1"
    default_port: int = 8000
