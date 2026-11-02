# MusicPlayer

基于 Qt 6.8.3 的桌面音乐播放器，支持本地播放、歌词、播放列表及网易云搜索/登录。Qt 客户端负责交互和播放，可选的 Python FastAPI 本地服务负责云接口与会话。

作者：misaka · [演示视频](https://www.bilibili.com/video/BV1tMwCz9ELE)

## 目录

```text
CMakeLists.txt           # 构建入口，输出 MusicPlayer
src/
  app/                  # main.cpp、AppConstants.h
  ui/                   # 主窗口、播放列表、菜单和自定义控件
  player/               # 播放控制、元数据池、歌词解析
  services/             # 云接口、播放列表存储、网络错误处理
ui/                     # Qt Designer .ui 表单
resources/
  res.qrc               # alias 保持 :/res/* 和 :/style.qss
  images/               # 图标和背景
  style.qss
python/server/          # FastAPI 入口 main.py、配置、依赖和辅助模块
tests/                 # 后端离线测试、客户端启动/资源测试
docs/                  # 开发指南、对象树与数据流、功能说明
.github/workflows/      # Linux 干净构建与后端测试
```

## 最短构建与启动

需要 C++17 编译器、CMake ≥ 3.21.1、Ninja、Qt ≥ 6.8.3（Widgets、Multimedia、Network）；CI 固定 Qt 6.8.3，Qt5 不受支持。

```bash
# Ubuntu 24.04 系统依赖；Qt 使用官方 Desktop 安装包，并选择 Qt Multimedia
sudo apt-get install build-essential cmake ninja-build libgl1-mesa-dev libxkbcommon-dev libxcb-cursor0 libpulse-dev libegl1 libasound2t64
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64
cmake --build build --parallel 2
./build/MusicPlayer
```

本地音乐放在可执行文件旁的 `MusicList/`，同名 `.lrc` 放在音乐旁。客户端播放列表、封面和歌词缓存仍位于可执行文件目录；请使用源码外或 `build/` 构建目录。

云功能需要 Python 3.9+，从仓库根目录启动：

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r python/server/requirements.txt
.venv/bin/python -m uvicorn python.server.main:app --host 127.0.0.1 --port 8000
```

后端也可从任意目录运行 `/path/to/repo/.venv/bin/python /path/to/repo/python/server/main.py`。`GET /health` 返回 `{"status":"ok"}`。Windows 使用 `.venv\Scripts\python.exe`。

后端日志和会话存放在用户数据目录，可用绝对路径 `MUSICPLAYER_DATA_DIR` 覆盖；默认 Linux 为 `~/.local/share/MusicPlayer`（遵循 `XDG_DATA_HOME`）、macOS 为 `~/Library/Application Support/MusicPlayer`、Windows 为 `%LOCALAPPDATA%\MusicPlayer`。pyncm 使用固定 SHA-256 的镜像 wheel，安装需要访问该镜像。

## 架构与开发

`MainWindow` 连接 UI 与 `PlayerController`；后者协调播放、`MediaPlayerPool` 元数据解析和 `LrcParser` 歌词。`PlaylistStore` 保存本地列表，`CloudMusicService` 调用 `127.0.0.1:8000` 的 Python 服务。目录按职责归档，现有调用关系保持不变。

- [开发指南：依赖、启动、持久化、验证与目录约定](docs/development.md)
- [架构：详细对象树、数据流和后端组件](docs/architecture.md)
- [现有功能](docs/features.md)

```bash
ctest --test-dir build --output-on-failure
.venv/bin/python -m pip install -r python/server/requirements-test.txt
PYTHONDONTWRITEBYTECODE=1 .venv/bin/python -m unittest discover -s tests -v
```

Linux CI 执行 Qt6 Release 干净构建、offscreen 主窗口/资源测试及 Python 3.9/3.12 离线后端测试，不需要真实账号或音频硬件。Windows/macOS GUI、实际音频设备和真实上游登录需在对应环境另行验证。
