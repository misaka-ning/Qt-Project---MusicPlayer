# 开发指南

[项目入口](../README.md) · [架构与数据流](architecture.md)

## 环境与依赖

- **Qt 版本**：Qt 6.8.3 起（Qt Widgets / Qt Multimedia / Qt Network），CI 固定验证 6.8.3。Qt5 不受支持：客户端使用 Qt6 的 `QAudioOutput`、`QMediaPlayer::setSource` 和 `QStringDecoder`。
- **构建系统**：CMake（最低 3.21.1；可搭配 Ninja 或 Visual Studio 生成器）
- **编译器**：任意支持 C++17 的编译器（例如 MSVC / Clang / GCC / MinGW）
- **Python（云功能必需）**：Python 3.9+，建议虚拟环境
- **Python 依赖（云功能）**：`fastapi>=0.115`、`uvicorn>=0.30`、`pyncm==1.8.1`（见 `python/server/requirements.txt`）。当前 PyPI 无可下载的 pyncm 分发，依赖文件使用腾讯 PyPI 镜像保存的 1.8.1 wheel 并校验 SHA-256；安装需能访问该镜像。

> 上述更完整的模块与持久化说明见上方「技术栈」小节。

---

## 编译与运行

1. **（可选）启动本地云服务（用于云搜索/网易云登录）**

   ```bash
   # 从仓库根目录创建环境（Windows 使用 .venv\Scripts\python.exe）
   python3 -m venv .venv
   .venv/bin/python -m pip install -r python/server/requirements.txt
   .venv/bin/python -m uvicorn python.server.main:app --host 127.0.0.1 --port 8000
   # 从任意工作目录启动（替换为实际绝对路径）
   /path/to/repo/.venv/bin/python /path/to/repo/python/server/main.py
   # 或：/path/to/repo/.venv/bin/python -m uvicorn main:app --app-dir /path/to/repo/python/server
   ```

   - 默认监听 `127.0.0.1:8000`，Qt 客户端通过该地址访问云接口；`GET /health` 应返回 `{"status":"ok"}`。
   - 后端默认数据目录：Linux `$XDG_DATA_HOME/MusicPlayer`（未设置时为 `~/.local/share/MusicPlayer`），macOS `~/Library/Application Support/MusicPlayer`，Windows `%LOCALAPPDATA%\MusicPlayer`。
   - 可在启动前设置 `MUSICPLAYER_DATA_DIR` 为绝对路径（例如 `export MUSICPLAYER_DATA_DIR=/tmp/musicplayer-data`，PowerShell：`$env:MUSICPLAYER_DATA_DIR="C:\Users\you\MusicPlayerData"`）。日志 `server.log` 和会话 `netease_session.json` 都写入该目录；目录按需创建。持久化失败在服务终端输出无凭据警告，服务仍可运行，但登录态可能无法跨重启保留。
   - 不自动读取仓库中旧会话，需重新登录；请勿提交会话或日志。已有 Git 历史不会因此清除。
   - 不启服务时，本地音乐播放仍可用，但云搜索/云播放/网易云登录不可用。

2. **准备音乐文件（本地播放）**

   - 在构建生成的可执行文件同级目录下创建 `MusicList` 文件夹，例如：
     - Windows：`<build-dir>\MusicPlayer.exe` 旁边创建 `MusicList\`
   - 将你的音频文件放到 `MusicList` 中（支持的格式见上）。
   - 如需歌词，在同一目录下放置同名 `.lrc` 文件，例如：
     - `song.mp3` 对应 `song.lrc`

3. **使用 CMake 构建 Qt 客户端**

   ```bash
   # 在项目根目录（包含 CMakeLists.txt）下
   # Qt 官方安装器选择 Desktop Qt 6.8.3 和 Qt Multimedia。
   # Linux 最小系统依赖（Ubuntu 24.04）：
   # sudo apt-get install build-essential cmake ninja-build libgl1-mesa-dev libxkbcommon-dev libxcb-cursor0 libpulse-dev libegl1 libasound2t64
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64
   cmake --build build --parallel 2
   ./build/MusicPlayer
   ```

> 说明：运行时会以 **可执行文件所在目录** 作为数据目录（`MusicList/`、`playlist.json`、`Metadata/` 均在该目录下创建/读取）。在 Windows 下，通常是 `build/<config>/` 目录中的 `MusicPlayer.exe` 所在位置。


### 无账号回归验证

```bash
.venv/bin/python -m pip install -r python/server/requirements-test.txt
PYTHONDONTWRITEBYTECODE=1 .venv/bin/python -m unittest discover -s tests -v
```

测试使用临时数据目录、模拟登录上游，覆盖真实本地 HTTP 启动、不同工作目录、会话读写/删除、损坏文件和写入失败，不需要账号或云端凭据。CI 在 Linux 执行 Qt 6.8.3 Release 干净构建及 Python 3.9/3.12 后端测试。Windows/macOS 桌面运行、实际音频设备/编解码器与真实网易云接口需在对应环境另行验证。

## 目录与修改约定

| 原位置 | 当前位置 | 内容 |
| --- | --- | --- |
| `main.cpp`、`config/AppConstants.h` | `src/app/` | 入口与应用常量 |
| 窗口、列表、菜单、自定义控件的 `.h/.cpp` | `src/ui/` | 界面行为 |
| `playercontroller.*`、`mediaplayerpool.*`、`lrcparser.*` | `src/player/` | 播放与歌词 |
| `cloudmusicservice.*`、`playliststore.*`、`utils/ErrorHandlingUtils.*` | `src/services/` | 云接口、存储与错误处理 |
| 根目录 `.ui` | `ui/` | Designer 表单 |
| `res/*`、`style.qss`、`res.qrc` | `resources/images/`、`resources/style.qss`、`resources/res.qrc` | 静态资源 |
| README 中详细对象树与数据流 | `docs/architecture.md` | 架构参考 |

新增客户端文件需显式加入根目录 `CMakeLists.txt` 的 `MusicPlayerClient` 源文件清单。
include 统一相对于 `src/`，例如 `#include "player/playercontroller.h"`；不要依赖所有子目录加入搜索路径。
`AUTOUIC_SEARCH_PATHS` 指向 `ui/`，生成的 `ui_*.h` 由构建目录提供，不提交生成文件。
Designer 自定义控件 header 使用相同 include 约定。

qrc 中磁盘路径相对于 `resources/res.qrc`，alias 保持原运行时 URL：图片为 `:/res/<原文件名>`，样式为 `:/style.qss`。新增资源时保留显式 alias，并更新资源测试的预期数量。
`MusicPlayerClient` 是对象库，生产程序和 smoke 测试复用相同窗口代码及资源对象，不增加发布库；程序名仍是 `MusicPlayer`。

## 客户端无硬件验证

```bash
ctest --test-dir build --output-on-failure
# 可选：只构建发布程序
cmake -S . -B build -DBUILD_TESTING=OFF
```

`client_startup_resources` 使用 Qt offscreen 平台，解码全部 22 张内嵌图片，读取 `:/style.qss`，显示真实 `MainWindow` 并运行事件循环后正常退出。
无需音乐、音频硬件或账号；不启动后端、不发起真实登录。测试可能在构建输出目录创建客户端缓存，不能把测试程序放在源码目录。
Linux CI 从 checkout 开始构建并检查源码树清洁。PR #1 的原始大小写错误复现已在该 PR 验证；目录调整后的 CI 持续验证正常构建和窗口资源，不再构造旧根目录布局。

个人 Qt Creator 配置 `CMakeLists.txt.user`、编辑器配置、构建产物及运行缓存不提交。已停止跟踪旧个人配置，本机文件保留；不要改写 Git 历史。
