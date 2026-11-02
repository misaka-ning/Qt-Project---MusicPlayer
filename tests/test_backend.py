"""Offline regressions: no real accounts, cookies or upstream requests."""
import importlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import Mock, patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))


class BackendTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.sandbox = tempfile.TemporaryDirectory()
        cls.env = patch.dict(os.environ, {"MUSICPLAYER_DATA_DIR": cls.sandbox.name})
        cls.env.start()
        cls.server = importlib.import_module("python.server.main")

    @classmethod
    def tearDownClass(cls):
        cls.env.stop()
        cls.sandbox.cleanup()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.data = Path(self.temp.name) / "data"
        self.config = self.server.AppConfig(data_dir=self.data)
        self.repo = self.server.SessionRepository(self.config)

    def test_session_roundtrip_clear_and_private_atomic_write(self):
        payload = {"cookies": [{"name": "MUSIC_U", "value": "fake-cookie"}]}
        with patch.object(self.server.ncm_login, "GetCurrentSession", return_value=Mock(dump=lambda: payload)):
            self.repo.save()
        self.assertTrue(self.config.session_file.is_file())
        self.assertEqual(list(self.data.glob('.session-*')), [])
        if os.name == "posix":
            self.assertEqual(self.config.session_file.stat().st_mode & 0o777, 0o600)
        with patch.object(self.server.ncm_login, "LoginViaCookie") as restore:
            self.repo.load()
            restore.assert_called_once_with(MUSIC_U="fake-cookie")
        self.repo.clear()
        self.assertFalse(self.config.session_file.exists())
        self.repo.load()  # A first run needs no session.

    def test_real_pyncm_dump_format_restores_without_network(self):
        from pyncm import Session
        session = Session()
        session.cookies.set("MUSIC_U", "fake-cookie")
        with patch.object(self.server.ncm_login, "GetCurrentSession", return_value=session):
            self.repo.save()
        restored = Session()
        with patch.object(self.server.ncm_login, "LoginViaCookie", side_effect=RuntimeError("offline")), patch.object(self.server.ncm_login, "GetCurrentSession", return_value=restored):
            self.repo.load()
        self.assertEqual(restored.cookies.get("MUSIC_U"), "fake-cookie")

    def test_persistence_errors_are_visible_and_redacted(self):
        self.data.mkdir()
        self.config.session_file.write_text('{fake-cookie: corrupt')
        with self.assertLogs("musicplayer.persistence", level="WARNING") as logs:
            self.repo.load()
        self.assertNotIn("fake-cookie", str(logs.output))
        with patch.object(self.server.ncm_login, "GetCurrentSession", side_effect=RuntimeError("fake-password")):
            with self.assertLogs("musicplayer.persistence", level="WARNING") as logs:
                self.repo.save()
        self.assertNotIn("fake-password", str(logs.output))
        self.config.session_file.unlink()
        self.config.session_file.mkdir()
        with self.assertLogs("musicplayer.persistence", level="WARNING"):
            self.repo.clear()

    def test_unwritable_data_directory(self):
        self.data.write_text("file blocks directory creation")
        with patch.object(self.server.ncm_login, "GetCurrentSession", return_value=Mock(dump=lambda: {})):
            with self.assertLogs("musicplayer.persistence", level="WARNING"):
                self.repo.save()
        with self.assertLogs("musicplayer.persistence", level="WARNING"):
            self.server.DebugLogger(self.config).write("test", "test", "test", {})

    def test_failed_replace_keeps_previous_session(self):
        self.data.mkdir()
        self.config.session_file.write_text('{}')
        with patch.object(self.server.ncm_login, "GetCurrentSession", return_value=Mock(dump=lambda: {})):
            with patch.object(self.server.os, "replace", side_effect=OSError("fake-secret")):
                with self.assertLogs("musicplayer.persistence", level="WARNING"):
                    self.repo.save()
        self.assertEqual(self.config.session_file.read_text(), '{}')
        self.assertEqual(list(self.data.glob('.session-*')), [])

    def test_mock_login_qr_logout_routes(self):
        from fastapi.testclient import TestClient
        server = self.server
        container = server.AppContainer()
        container.session_repo = self.repo
        container.auth_service = server.AuthService(self.repo)
        container.qr_auth_service = server.QrAuthService(container.gateway, self.repo, server.DebugLogger(self.config), container.auth_service)
        with patch.object(server, "container", container), patch.object(server, "call_pyncm_no_proxy", return_value={"code": 200}), patch.object(container.auth_service, "get_login_status", return_value=(True, "test-user")), patch.object(server.ncm_login, "GetCurrentSession", return_value=Mock(dump=lambda: {})):
            with TestClient(server.app) as client:
                self.assertEqual(client.get('/health').json(), {"status": "ok"})
                self.assertEqual(client.post('/auth/login', json={"phone": "fake", "password": "fake"}).json()['code'], 0)
                self.assertTrue(self.config.session_file.exists())
                with patch.object(container.qr_auth_service, 'qr_url', return_value='https://example.invalid/?key=fake-secret'):
                    self.assertEqual(client.get('/auth/qr/url', params={'unikey': 'fake'}).json()['code'], 0)
                self.assertNotIn('fake-secret', self.config.debug_log_path.read_text())
                self.assertEqual(client.post('/auth/logout').json()['code'], 0)
                self.assertFalse(self.config.session_file.exists())

    def test_start_from_different_working_directories(self):
        # Real uvicorn process and HTTP request, with all upstream networking forbidden.
        runner = r'''
import importlib, os, runpy, socket, sys, threading, urllib.request
from unittest.mock import patch
from pathlib import Path
root, mode = sys.argv[1:]
sys.path.insert(0, root)
with patch('socket.socket.connect', side_effect=AssertionError('upstream forbidden')):
    if mode == 'script':
        sys.path.insert(0, str(Path(root) / 'python/server'))
        import uvicorn
        with patch.object(uvicorn, 'run'):
            ns = runpy.run_path(str(Path(root) / 'python/server/main.py'), run_name='__main__')
        app = ns['app']
    else:
        app = importlib.import_module('python.server.main').app
import uvicorn
sock = socket.socket()
sock.bind(('127.0.0.1', 0))
server = uvicorn.Server(uvicorn.Config(app, log_level='error'))
thread = threading.Thread(target=server.run, kwargs={'sockets': [sock]})
thread.start()
try:
    import time
    for _ in range(200):
        if server.started: break
        time.sleep(.01)
    with urllib.request.build_opener(urllib.request.ProxyHandler({})).open('http://127.0.0.1:%s/health' % sock.getsockname()[1], timeout=5) as r:
        assert r.status == 200
finally:
    server.should_exit = True
    thread.join(timeout=10)
    assert not thread.is_alive()
'''
        def source_files():
            return {str(p.relative_to(ROOT)): (p.stat().st_size, p.stat().st_mtime_ns)
                    for p in ROOT.rglob('*') if p.is_file() and '.git' not in p.relative_to(ROOT).parts}
        before = source_files()
        for mode, cwd in [('module', ROOT), ('script', Path(self.temp.name))]:
            env = dict(os.environ, MUSICPLAYER_DATA_DIR=str(self.data), PYTHONDONTWRITEBYTECODE='1')
            result = subprocess.run([sys.executable, '-c', runner, str(ROOT), mode], cwd=cwd, env=env, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(source_files(), before)
        self.assertFalse(self.data.exists())
        self.assertEqual(list(Path(self.temp.name).iterdir()), [])

    def test_default_paths_and_override(self):
        from python.server.config import user_data_dir
        with patch.dict(os.environ, {'MUSICPLAYER_DATA_DIR': 'relative'}):
            with self.assertRaises(ValueError):
                user_data_dir()
        with patch.dict(os.environ, {'MUSICPLAYER_DATA_DIR': str(self.data)}):
            self.assertEqual(user_data_dir(), self.data)


if __name__ == '__main__':
    unittest.main()
