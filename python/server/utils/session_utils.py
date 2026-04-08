from __future__ import annotations

from typing import Any


def extract_music_u_from_payload(payload: dict[str, Any]) -> str:
    cookies = payload.get("cookies")
    if not isinstance(cookies, list):
        return ""
    for cookie in cookies:
        if not isinstance(cookie, dict):
            continue
        if str(cookie.get("name", "")).strip() != "MUSIC_U":
            continue
        music_u = str(cookie.get("value", "")).strip()
        if music_u:
            return music_u
    return ""
