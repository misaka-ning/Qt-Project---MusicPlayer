from __future__ import annotations

from typing import Any


def is_success_payload(payload: Any, success_codes: tuple[int, ...] = (0, 200)) -> bool:
    if not isinstance(payload, dict):
        return False
    return int(payload.get("code", -1)) in success_codes


def extract_payload_message(payload: Any, fallback: str) -> str:
    if isinstance(payload, dict):
        message = payload.get("message")
        if isinstance(message, str) and message.strip():
            return message
    return fallback
