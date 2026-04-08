"""
本地云音乐 API，与 Qt 端 CloudMusicService 约定一致：
  GET /search?keyword=&page=&size=
  GET /song/url?id=
  POST /auth/login  JSON: phone, password, countrycode(默认 86)
  GET /auth/status
  POST /auth/logout

搜索：直连 music.163.com/api/cloudsearch/pc（含 fee、st，与 privileges 对齐）。
播放地址：apis.track.GetTrackAudio；无 URL 时按详情 fee/st 返回 HTTP 200 + code 403。
登录：pyncm 会话写入其默认存储；请在固定工作目录下启动本服务，以免登录态丢失。

依赖：pip install -r requirements.txt
运行：python main.py 或 uvicorn main:app --host 127.0.0.1 --port 8000
"""

from __future__ import annotations

import json
import os
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from typing import Any, Union

from fastapi import FastAPI, Query, Request
from fastapi.responses import JSONResponse, StreamingResponse
from pydantic import BaseModel, Field

from pyncm import apis
from pyncm.apis import login as ncm_login
from pyncm.apis.exception import LoginFailedException


@dataclass(frozen=True)
class AppConfig:
    """
    集中配置（Configuration Object）。
    通过单一配置对象统一管理路径/UA/域名等常量，避免散落在函数中难以维护。
    """

    user_agent: str = (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"
    )
    referer: str = "https://music.163.com/"
    debug_log_path: str = r"D:\Qt\MusicPlayer\debug-502184.log"
    session_file: str = os.path.join(os.path.dirname(__file__), "netease_session.json")


class ResponseFactory:
    """
    统一 API 响应构造（Factory Method 风格）。
    目标：让返回结构稳定，路由函数只关心业务，不关心 JSON 拼装细节。
    """

    @staticmethod
    def ok(data: dict[str, Any] | None = None, message: str = "ok") -> dict[str, Any]:
        return {"code": 0, "message": message, "data": data or {}}

    @staticmethod
    def fail_json(status: int, message: str) -> JSONResponse:
        return JSONResponse(status_code=status, content={"code": status, "message": message, "data": {}})


class SessionRepository:
    """
    登录会话仓储（Repository）。
    负责把 pyncm 登录态与本地文件互转，屏蔽序列化/兼容逻辑。
    """

    def __init__(self, config: AppConfig) -> None:
        self._config = config

    def save(self) -> None:
        """将当前 pyncm 会话持久化到本地文件。"""
        try:
            session = ncm_login.GetCurrentSession()
            payload = session.dump() if session is not None else {}
            music_u = ""
            if isinstance(payload, dict):
                cookies = payload.get("cookies")
                if isinstance(cookies, list):
                    for c in cookies:
                        if not isinstance(c, dict):
                            continue
                        if str(c.get("name", "")).strip() == "MUSIC_U":
                            music_u = str(c.get("value", "")).strip()
                            if music_u:
                                break
            out = {"music_u": music_u, "session_dump": payload}
            with open(self._config.session_file, "w", encoding="utf-8") as f:
                json.dump(out, f, ensure_ascii=False)
        except Exception:
            pass

    def load(self) -> None:
        """启动时恢复上次登录会话，失败时静默降级为未登录。"""
        if not os.path.exists(self._config.session_file):
            return
        try:
            with open(self._config.session_file, "r", encoding="utf-8") as f:
                raw = json.load(f)

            music_u = ""
            payload: dict[str, Any] = {}
            if isinstance(raw, dict) and "session_dump" in raw:
                # 新格式：{music_u, session_dump}
                music_u = str(raw.get("music_u") or "").strip()
                sd = raw.get("session_dump")
                if isinstance(sd, dict):
                    payload = sd
            elif isinstance(raw, dict):
                # 兼容旧格式：直接 session.dump()
                payload = raw
                cookies = raw.get("cookies")
                if isinstance(cookies, list):
                    for c in cookies:
                        if not isinstance(c, dict):
                            continue
                        if str(c.get("name", "")).strip() == "MUSIC_U":
                            music_u = str(c.get("value", "")).strip()
                            if music_u:
                                break

            # 优先使用 MUSIC_U 恢复（跨进程重启更稳定）
            if music_u:
                try:
                    ncm_login.LoginViaCookie(MUSIC_U=music_u)
                    return
                except Exception:
                    pass

            # 兜底：尝试恢复完整会话
            if payload:
                ncm_login.WriteLoginInfo(payload)
        except Exception:
            pass

    def clear(self) -> None:
        try:
            if os.path.exists(self._config.session_file):
                os.remove(self._config.session_file)
        except Exception:
            pass


class NeteaseHttpClient:
    """
    直连网易云 HTTP 客户端（Adapter）。
    统一处理 headers、禁用系统代理、超时控制。
    """

    def __init__(self, config: AppConfig) -> None:
        self._config = config

    def get_json(self, url: str, timeout: float = 12.0) -> Any:
        """直连上游，不使用环境变量里的 HTTP(S)_PROXY，避免本机代理未启动时出现 WinError 10061。"""
        req = urllib.request.Request(url, headers={"User-Agent": self._config.user_agent, "Referer": self._config.referer})
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        with opener.open(req, timeout=timeout) as resp:
            raw = resp.read().decode("utf-8", errors="replace")
        return json.loads(raw)

    def open_stream(self, url: str, request_range: str | None):
        """
        打开上游音频流并返回原始响应对象。
        保留对 Range 的透传，确保 Qt 播放器 seek 时可触发 206 分片拉流。
        """
        upstream_headers = {"User-Agent": self._config.user_agent, "Referer": self._config.referer}
        if isinstance(request_range, str) and request_range.strip():
            # 透传 Range，允许 QMediaPlayer/FFmpeg 走 206 分段请求实现 seek。
            upstream_headers["Range"] = request_range.strip()
        req = urllib.request.Request(url, headers=upstream_headers)
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        return opener.open(req, timeout=20.0)


class SongMapper:
    """歌曲数据映射器（Mapper），负责上游字段 -> Qt 侧契约字段。"""

    @staticmethod
    def safe_int(v: Any, default: int = 0) -> int:
        try:
            return int(v)
        except (TypeError, ValueError):
            return default

    @classmethod
    def map_cloud_song(cls, s: dict[str, Any], priv: dict[str, Any] | None = None) -> dict[str, Any]:
        artists = s.get("ar") or []
        artist_str = "/".join(str(a.get("name", "")) for a in artists if isinstance(a, dict))
        al = s.get("al") if isinstance(s.get("al"), dict) else {}
        pic = al.get("picUrl") or ""
        if isinstance(pic, str) and pic.startswith("http://"):
            pic = "https://" + pic[len("http://") :]
        fee = cls.safe_int(s.get("fee", 0), 0)
        st_val: int | None = None
        if priv is not None and isinstance(priv, dict) and "st" in priv:
            st_val = cls.safe_int(priv.get("st"), 0)
        if st_val is None:
            st_val = cls.safe_int(s.get("st", 0), 0)
        return {
            "id": str(s.get("id", "")),
            "name": str(s.get("name", "")),
            "artist": artist_str,
            "album": str(al.get("name", "")),
            "duration": int(s.get("dt", 0) or 0),
            "cover": pic,
            "fee": fee,
            "st": st_val,
        }


class NeteaseGateway:
    """
    第三方调用门面（Facade）。
    将 pyncm 和 raw HTTP 的调用细节集中，路由与业务层只依赖此门面。
    """

    def __init__(self, http_client: NeteaseHttpClient) -> None:
        self._http = http_client

    def search_cloud(self, keyword: str, page: int, size: int) -> Any:
        offset = (page - 1) * size
        q = urllib.parse.urlencode({"type": "1", "s": keyword, "offset": str(offset), "limit": str(size)})
        return self._http.get_json(f"https://music.163.com/api/cloudsearch/pc?{q}")

    def get_track_audio(self, song_id: int) -> Any:
        return apis.track.GetTrackAudio(song_id, bitrate=320000, encodeType="mp3")

    def get_track_detail(self, song_id: int) -> Any:
        return apis.track.GetTrackDetail(song_id)

    def get_track_lyrics(self, song_id: int) -> Any:
        getter = getattr(apis.track, "GetTrackLyrics", None) or getattr(apis.track, "GetTrackLyric", None)
        if getter is None:
            raise RuntimeError("pyncm lyric api unavailable")
        return getter(song_id)

    def stream_upstream(self, src: str, request_range: str | None):
        return self._http.open_stream(src, request_range)

    def call_login_api_compatible(self, fn_names: list[str], *args: Any, **kwargs: Any) -> Any:
        """
        pyncm 不同版本/分支中二维码登录 API 命名与参数可能不同；
        这里按候选名依次尝试调用，尽量保证兼容。
        """
        last_err: Exception | None = None
        for name in fn_names:
            fn = getattr(ncm_login, name, None) or getattr(apis.login, name, None)
            if fn is None:
                continue
            try:
                return fn(*args, **kwargs)
            except TypeError as e:
                last_err = e
                continue
        if last_err is not None:
            raise last_err
        raise AttributeError("No compatible pyncm qr-login API found")


class DebugLogger:
    """轻量日志器，保留原 agent log 行为。"""

    def __init__(self, config: AppConfig) -> None:
        self._config = config

    def write(self, location: str, message: str, hypothesis_id: str, data: dict[str, Any]) -> None:
        # region agent log
        try:
            with open(self._config.debug_log_path, "a", encoding="utf-8") as f:
                f.write(
                    json.dumps(
                        {
                            "sessionId": "502184",
                            "runId": "pre-fix",
                            "hypothesisId": hypothesis_id,
                            "location": location,
                            "message": message,
                            "data": data,
                            "timestamp": int(time.time() * 1000),
                        },
                        ensure_ascii=False,
                    )
                    + "\n"
                )
        except Exception:
            pass
        # endregion


class MusicService:
    """音乐查询与播放服务（Service）。"""

    def __init__(self, gateway: NeteaseGateway) -> None:
        self._gateway = gateway

    @staticmethod
    def _playback_denied_message(fee: int, st: int) -> str:
        if st < 0:
            return "该歌曲无版权或当前地区不可播放"
        if fee == 4:
            return "该歌曲需要购买数字专辑或单曲"
        if fee == 1:
            return "该歌曲需要网易云音乐 VIP，请在菜单中登录账号后重试"
        return "当前无法获取播放地址，请尝试登录网易云账号或检查网络"

    def search(self, keyword: str, page: int, size: int) -> Union[dict[str, Any], JSONResponse]:
        """搜索歌曲，支持分页。"""
        kw = keyword.strip()
        if not kw:
            return ResponseFactory.fail_json(400, "invalid search parameters")

        try:
            data = self._gateway.search_cloud(kw, page, size)
        except urllib.error.HTTPError as e:
            return ResponseFactory.fail_json(e.code, f"netease http {e.code}")
        except urllib.error.URLError as e:
            reason = getattr(e.reason, "strerror", None) or str(e.reason)
            return ResponseFactory.fail_json(
                502,
                f"cannot reach netease search: {reason}. "
                "If you use a system proxy, ensure it is running or unset HTTP_PROXY/HTTPS_PROXY.",
            )

        if not isinstance(data, dict):
            return ResponseFactory.fail_json(502, "invalid netease response")
        if data.get("code") != 200:
            return ResponseFactory.fail_json(502, str(data.get("message") or data.get("msg") or "search failed"))

        result = data.get("result")
        if not isinstance(result, dict):
            return ResponseFactory.fail_json(502, "invalid search result")

        songs_raw = result.get("songs") or []
        privileges = result.get("privileges") or []
        songs: list[dict[str, Any]] = []
        if isinstance(songs_raw, list):
            for i, s in enumerate(songs_raw):
                if not isinstance(s, dict) or not s.get("id"):
                    continue
                priv = privileges[i] if isinstance(privileges, list) and i < len(privileges) and isinstance(privileges[i], dict) else None
                songs.append(SongMapper.map_cloud_song(s, priv))
        return ResponseFactory.ok({"songs": songs})

    def fetch_track_detail_brief(self, song_id: int) -> dict[str, str] | None:
        """调用 GetTrackDetail，将首条歌曲格式化为与搜索接口一致的扁平字段；失败返回 None。"""
        try:
            raw = self._gateway.get_track_detail(song_id)
        except Exception:
            return None
        if not isinstance(raw, dict) or SongMapper.safe_int(raw.get("code", -1), -1) != 200:
            return None
        songs = raw.get("songs")
        if not isinstance(songs, list) or not songs or not isinstance(songs[0], dict):
            return None
        privileges = raw.get("privileges") or []
        first_priv = privileges[0] if isinstance(privileges, list) and privileges and isinstance(privileges[0], dict) else None
        m = SongMapper.map_cloud_song(songs[0], first_priv)
        return {"name": m["name"], "artist": m["artist"], "album": m["album"], "cover": m["cover"]}

    def detail_fee_st(self, song_id: int) -> tuple[int, int]:
        """从 GetTrackDetail 取首条歌曲的 fee 与 st（优先 privileges[0].st）。"""
        try:
            raw = self._gateway.get_track_detail(song_id)
        except Exception:
            return (0, 0)
        if not isinstance(raw, dict) or SongMapper.safe_int(raw.get("code", -1), -1) != 200:
            return (0, 0)
        songs = raw.get("songs")
        if not isinstance(songs, list) or not songs or not isinstance(songs[0], dict):
            return (0, 0)
        privileges = raw.get("privileges") or []
        priv0 = privileges[0] if isinstance(privileges, list) and privileges and isinstance(privileges[0], dict) else None
        mapped = SongMapper.map_cloud_song(songs[0], priv0)
        return (SongMapper.safe_int(mapped.get("fee", 0), 0), SongMapper.safe_int(mapped.get("st", 0), 0))

    def song_url(self, song_id: str) -> Union[dict[str, Any], JSONResponse]:
        """按歌曲 id 取可播放 URL；成功后再取详情，在 data 中返回 name、artist 等供 Qt 更新标题。"""
        sid = song_id.strip()
        if not sid.isdigit():
            return ResponseFactory.fail_json(400, "invalid song id")
        sid_i = int(sid)

        try:
            raw = self._gateway.get_track_audio(sid_i)
        except Exception as e:
            return ResponseFactory.fail_json(502, str(e) or "get track audio failed")

        if not isinstance(raw, dict):
            return ResponseFactory.fail_json(502, "invalid pyncm response")
        if SongMapper.safe_int(raw.get("code", -1), -1) != 200:
            return ResponseFactory.fail_json(502, str(raw.get("message") or raw.get("msg") or "netease player url error"))

        arr = raw.get("data")
        if not isinstance(arr, list) or not arr or not isinstance(arr[0], dict):
            return ResponseFactory.fail_json(502, "no audio data")

        u = arr[0].get("url")
        if not isinstance(u, str) or not u.strip():
            fee, st = self.detail_fee_st(sid_i)
            return {"code": 403, "message": self._playback_denied_message(fee, st), "data": {}}

        u = u.strip()
        if u.startswith("http://"):
            u = "https://" + u[len("http://") :]

        stream_q = urllib.parse.urlencode({"id": sid, "src": u})
        # 某些环境下代理流可能被上游 403，主 url 先返回直连，代理地址作为备用。
        data_out: dict[str, Any] = {
            "url": u,
            "originUrl": u,
            "proxyUrl": f"http://127.0.0.1:8000/song/stream?{stream_q}",
        }
        brief = self.fetch_track_detail_brief(sid_i)
        if brief:
            data_out.update(brief)
        return ResponseFactory.ok(data_out)

    def stream(self, request: Request, song_id: str, src: str):
        """代理音频流，给上游补齐 User-Agent/Referer，规避客户端直连 403。"""
        sid = song_id.strip()
        if not sid.isdigit():
            return ResponseFactory.fail_json(400, "invalid song id")
        upstream = src.strip()
        if not (upstream.startswith("http://") or upstream.startswith("https://")):
            return ResponseFactory.fail_json(400, "invalid source url")

        try:
            resp = self._gateway.stream_upstream(upstream, request.headers.get("range"))
        except urllib.error.HTTPError as e:
            return ResponseFactory.fail_json(e.code, f"stream upstream http {e.code}")
        except urllib.error.URLError as e:
            return ResponseFactory.fail_json(502, f"stream upstream error: {e.reason}")

        content_type = resp.headers.get("Content-Type", "audio/mpeg")
        content_length = resp.headers.get("Content-Length")
        content_range = resp.headers.get("Content-Range")
        status_code = getattr(resp, "status", None) or 200
        response_headers = {"Accept-Ranges": "bytes"}
        if isinstance(content_length, str) and content_length.strip():
            response_headers["Content-Length"] = content_length.strip()
        if isinstance(content_range, str) and content_range.strip():
            response_headers["Content-Range"] = content_range.strip()

        def iter_chunks():
            try:
                while True:
                    chunk = resp.read(64 * 1024)
                    if not chunk:
                        break
                    yield chunk
            finally:
                resp.close()

        return StreamingResponse(iter_chunks(), media_type=content_type, status_code=status_code, headers=response_headers)

    def lyrics(self, song_id: str) -> Union[dict[str, Any], JSONResponse]:
        """按歌曲 id 返回歌词文本，字段名与 Qt 端约定：lyrics。"""
        sid = song_id.strip()
        if not sid.isdigit():
            return ResponseFactory.fail_json(400, "invalid song id")
        try:
            raw = self._gateway.get_track_lyrics(int(sid))
        except RuntimeError as e:
            return ResponseFactory.fail_json(500, str(e))
        except Exception as e:
            return ResponseFactory.fail_json(502, str(e) or "get lyric failed")
        if not isinstance(raw, dict):
            return ResponseFactory.fail_json(502, "invalid lyric response")
        lrc_obj = raw.get("lrc")
        if not isinstance(lrc_obj, dict):
            return ResponseFactory.ok({"lyrics": ""})
        return ResponseFactory.ok({"lyrics": str(lrc_obj.get("lyric") or "")})


class AuthService:
    """账号密码登录相关服务。"""

    def __init__(self, session_repo: SessionRepository) -> None:
        self._session_repo = session_repo

    @staticmethod
    def get_login_status() -> tuple[bool, str]:
        try:
            raw = ncm_login.GetCurrentLoginStatus()
        except Exception:
            return (False, "")
        if not isinstance(raw, dict) or SongMapper.safe_int(raw.get("code", -1), -1) != 200:
            return (False, "")
        profile = raw.get("profile")
        if not isinstance(profile, dict):
            return (False, "")
        uid = profile.get("userId")
        nickname = str(profile.get("nickname") or profile.get("nickName") or "")
        logged_in = uid is not None and str(uid) not in ("0", "", "None")
        return (logged_in, nickname)

    def login_with_password(self, phone: str, password: str, countrycode: int) -> dict[str, Any]:
        phone_s = phone.strip()
        if not phone_s:
            return {"code": 400, "message": "手机号为空", "data": {}}
        try:
            ncm_login.LoginViaCellphone(phone=phone_s, password=password, ctcode=countrycode)
        except LoginFailedException as e:
            return {"code": 401, "message": str(e) or "登录失败", "data": {}}
        except Exception as e:
            return {"code": 500, "message": str(e) or "登录异常", "data": {}}
        _, nickname = self.get_login_status()
        self._session_repo.save()
        return ResponseFactory.ok({"nickname": nickname})

    def status(self) -> dict[str, Any]:
        logged_in, nickname = self.get_login_status()
        return ResponseFactory.ok({"loggedIn": logged_in, "nickname": nickname})

    def logout(self) -> dict[str, Any]:
        try:
            ncm_login.LoginLogout()
        except Exception:
            pass
        self._session_repo.clear()
        return ResponseFactory.ok({})


class QrAuthService:
    """二维码登录服务，包含多版本 API 兼容与状态翻译。"""

    def __init__(self, gateway: NeteaseGateway, session_repo: SessionRepository, logger: DebugLogger, auth_service: AuthService) -> None:
        self._gateway = gateway
        self._session_repo = session_repo
        self._logger = logger
        self._auth_service = auth_service

    def qr_unikey(self) -> str:
        raw = self._gateway.call_login_api_compatible(["LoginQrcodeUnikey", "LoginQRCodeUnikey"])
        if isinstance(raw, dict) and SongMapper.safe_int(raw.get("code", -1), -1) == 200:
            k = raw.get("unikey")
            if not (isinstance(k, str) and k.strip()) and isinstance(raw.get("data"), dict):
                k = raw["data"].get("unikey")
            if isinstance(k, str) and k.strip():
                return k.strip()
        if isinstance(raw, str) and raw.strip():
            return raw.strip()
        raise RuntimeError("failed to get qrcode unikey")

    def qr_url(self, unikey: str) -> str:
        # 常见命名：GetLoginQRCodeUrl / GetLoginQrcodeUrl；参数可能是 key/unikey/codekey
        for k in ("key", "unikey", "codekey"):
            try:
                raw = self._gateway.call_login_api_compatible(
                    ["GetLoginQRCodeUrl", "GetLoginQrcodeUrl", "GetLoginQRCodeURL"], **{k: unikey}
                )
                if isinstance(raw, dict) and SongMapper.safe_int(raw.get("code", -1), -1) == 200:
                    u = raw.get("qrurl") or raw.get("qrUrl") or raw.get("url")
                    if isinstance(u, str) and u.strip():
                        return u.strip()
                if isinstance(raw, str) and raw.strip():
                    return raw.strip()
            except TypeError:
                continue
        raise RuntimeError("failed to get qrcode url")

    def qr_check(self, unikey: str) -> dict[str, Any]:
        # 参数名通常是 key，但也可能接受 unikey/codekey
        for k in ("key", "unikey", "codekey"):
            try:
                raw = self._gateway.call_login_api_compatible(["LoginQrcodeCheck", "LoginQRCodeCheck"], **{k: unikey})
                if isinstance(raw, dict):
                    return {
                        "code": SongMapper.safe_int(raw.get("code", -1), -1),
                        "message": str(raw.get("message") or raw.get("msg") or ""),
                        "raw": raw,
                    }
            except TypeError:
                continue
        return {"code": -1, "message": "invalid qr check response"}

    @staticmethod
    def qr_status_from_code(code_i: int) -> tuple[str, str]:
        # 网易云二维码登录常见状态码：
        # 800 过期，801 等待扫码，802 已扫码待确认，803 已授权登录成功
        if code_i == 801:
            return ("waiting", "等待扫码")
        if code_i == 802:
            return ("scanned", "已扫码，请在手机上确认登录")
        if code_i == 803:
            return ("authorized", "登录成功")
        if code_i == 800:
            return ("expired", "二维码已过期，请刷新")
        return ("unknown", "状态未知")

    def api_unikey(self) -> dict[str, Any]:
        key = self.qr_unikey()
        self._logger.write(
            "python/server/main.py:auth_qr_unikey",
            "generated unikey",
            "H1",
            {"unikey_len": len(key)},
        )
        return ResponseFactory.ok({"unikey": key})

    def api_url(self, unikey: str) -> dict[str, Any]:
        key = unikey.strip()
        url = self.qr_url(key)
        self._logger.write(
            "python/server/main.py:auth_qr_url",
            "qr url returned",
            "H1",
            {"qrUrl": url[:160], "qrUrl_len": len(url)},
        )
        return ResponseFactory.ok({"unikey": key, "qrUrl": url})

    def api_check(self, unikey: str) -> dict[str, Any]:
        key = unikey.strip()
        chk = self.qr_check(key)
        ne_code = SongMapper.safe_int(chk.get("code", -1), -1)
        status, status_msg = self.qr_status_from_code(ne_code)

        logged_in = False
        nickname = ""
        if status == "authorized":
            logged_in, nickname = self._auth_service.get_login_status()
            if logged_in:
                self._session_repo.save()

        upstream_msg = str(chk.get("message") or "")
        out_msg = upstream_msg if upstream_msg else status_msg

        self._logger.write(
            "python/server/main.py:auth_qr_check",
            "qr check status",
            "H2",
            {"status": status, "statusCode": ne_code, "loggedIn": logged_in, "nickname_len": len(nickname or "")},
        )
        return ResponseFactory.ok(
            {
                "unikey": key,
                "status": status,
                "statusCode": ne_code,
                "statusMessage": out_msg,
                "loggedIn": logged_in,
                "nickname": nickname,
            }
        )


class AppContainer:
    """
    简易依赖注入容器（Composition Root）。
    统一组装对象依赖，路由仅通过 container 获取服务对象。
    """

    def __init__(self) -> None:
        self.config = AppConfig()
        self.http_client = NeteaseHttpClient(self.config)
        self.gateway = NeteaseGateway(self.http_client)
        self.logger = DebugLogger(self.config)
        self.session_repo = SessionRepository(self.config)
        self.music_service = MusicService(self.gateway)
        self.auth_service = AuthService(self.session_repo)
        self.qr_auth_service = QrAuthService(self.gateway, self.session_repo, self.logger, self.auth_service)
        self.session_repo.load()


class LoginBody(BaseModel):
    phone: str = Field(..., min_length=1)
    password: str = Field(..., min_length=1)
    countrycode: int = 86


container = AppContainer()
app = FastAPI(title="MusicPlayer Cloud API (NetEase)", version="2.1.0")


@app.get("/health")
def health() -> dict[str, str]:
    """健康检查接口"""
    return {"status": "ok"}


@app.get("/search", response_model=None)
def search(
    keyword: str = Query(..., min_length=1),
    page: int = Query(1, ge=1),
    size: int = Query(20, ge=1, le=100),
) -> Union[dict[str, Any], JSONResponse]:
    return container.music_service.search(keyword, page, size)


@app.get("/song/url", response_model=None)
def song_url(song_id: str = Query(..., alias="id", min_length=1)) -> Union[dict[str, Any], JSONResponse]:
    return container.music_service.song_url(song_id)


@app.get("/song/stream")
def song_stream(
    request: Request,
    song_id: str = Query(..., alias="id", min_length=1),
    src: str = Query(..., min_length=8),
):
    return container.music_service.stream(request, song_id, src)


@app.get("/lyrics", response_model=None)
def lyrics(song_id: str = Query(..., alias="songId", min_length=1)) -> Union[dict[str, Any], JSONResponse]:
    return container.music_service.lyrics(song_id)


@app.post("/auth/login", response_model=None)
def auth_login(body: LoginBody) -> dict[str, Any]:
    return container.auth_service.login_with_password(body.phone, body.password, body.countrycode)


@app.get("/auth/status", response_model=None)
def auth_status() -> dict[str, Any]:
    return container.auth_service.status()


@app.post("/auth/logout", response_model=None)
def auth_logout() -> dict[str, Any]:
    return container.auth_service.logout()


@app.post("/auth/qr/unikey", response_model=None)
def auth_qr_unikey() -> dict[str, Any]:
    try:
        return container.qr_auth_service.api_unikey()
    except Exception as e:
        return {"code": 500, "message": str(e) or "获取 unikey 失败", "data": {}}


@app.get("/auth/qr/url", response_model=None)
def auth_qr_url(unikey: str = Query(..., min_length=1)) -> dict[str, Any]:
    try:
        return container.qr_auth_service.api_url(unikey)
    except Exception as e:
        return {"code": 500, "message": str(e) or "获取二维码 URL 失败", "data": {}}


@app.get("/auth/qr/check", response_model=None)
def auth_qr_check(unikey: str = Query(..., min_length=1)) -> dict[str, Any]:
    try:
        return container.qr_auth_service.api_check(unikey)
    except Exception as e:
        return {"code": 500, "message": str(e) or "二维码状态查询失败", "data": {}}


if __name__ == "__main__":
    import uvicorn

    uvicorn.run(app, host="127.0.0.1", port=8000)
