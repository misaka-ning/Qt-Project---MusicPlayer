# MusicPlayer

一个基于 **Qt 6.8.3** 的本地音乐播放器，支持播放本地音频文件、展示封面和歌曲信息、歌词滚动显示、列表循环 / 随机播放 / 单曲循环等功能。UI 使用 Qt Widgets 实现，整体风格简单清爽。

> 作者：**misaka**  
> 项目类型：个人学习 / 练手项目

---

## 功能介绍

- **本地音乐扫描**
  - 启动后自动扫描可执行文件同目录下的 `MusicList` 文件夹。
  - 支持的音频格式：`mp3 / wav / flac / aac / ogg / m4a / wma`。

- **播放控制**
  - 播放 / 暂停、上一首、下一首。
  - 三种播放模式：
    - **列表循环**（List_Play）
    - **随机播放**（Loop_Play，内部使用洗牌算法生成播放顺序）
    - **单曲循环**（Repeat_Play）

- **播放列表**
  - 使用自定义 `MusicPlaylist` 窗口显示所有歌曲。
  - 每一行是一个 `SongUnit`，包含封面、歌曲名、艺术家。
  - 鼠标悬停高亮、点击即可切换播放。

- **封面与元数据**
  - 利用 `MediaPlayerPool` 对象池异步解析音频元数据。
  - 从音频文件中读取封面、标题、艺术家等信息，更新到播放列表和主界面。

- **歌词显示**
  - 支持 `.lrc` 歌词文件（与音频文件同名、同目录）。
  - 支持多种编码自动识别（UTF‑8、GBK、GB2312、GB18030、UTF‑16 等）。
  - 通过 `LrcParser` 解析时间戳，`QListWidget` 同步高亮当前行并居中滚动显示。

- **UI 与交互**
  - 控制面板采用圆角白色卡片风格。
  - 中间显示专辑封面和歌曲名（加粗、大号字体）。
  - 歌曲名、艺术家名支持“跑马灯”滚动效果（`MarqueeLabel`），适合长文本显示。
  - 播放进度条支持拖动，并在播放结束时自动切换到下一首。

---

## 项目结构

核心文件简要说明：

- `main.cpp`  
  应用入口，创建 `QApplication` 并显示 `MainWindow`。

- `mainwindow.h / mainwindow.cpp`  
  主窗口类，负责：
  - 初始化窗口、按钮、UI 样式。
  - 创建并摆放 `MusicPlaylist`。
  - 连接 UI 与 `PlayerController` 的信号槽（按钮、进度条、歌词等）。
  - 处理元数据更新、歌词显示、窗口大小变化等。

- `playercontroller.h / playercontroller.cpp`  
  **播放控制器（PlayerController）**：
  - 持有 `QMediaPlayer` 和 `QAudioOutput`。
  - 扫描 `MusicList` 目录，填充 `MusicPlaylist`。
  - 通过 `MediaPlayerPool` 异步解析封面和标签。
  - 统一管理播放模式（列表循环 / 随机 / 单曲）、上一首 / 下一首 / 单曲循环、“自动下一首”等逻辑。
  - 对外暴露简单接口（`PlayPrevSong / PlayNextSong / PlaySong / MusicEnd / SetPlayMode / GetPlayMode` 等），减轻 `MainWindow` 负担。

- `musicplaylist.h / musicplaylist.cpp / musicplaylist.ui`  
  播放列表窗口：
  - 内部使用 `QScrollArea + QVBoxLayout` 管理多个 `SongUnit`。
  - 提供追加歌曲、更新某一项封面/标题/艺术家等接口。
  - 通过信号 `ChooseMusicpass(int id)` 告诉 `PlayerController` 用户选择了哪一首。

- `songunit.h / songunit.cpp / songunit.ui`  
  播放列表中的一行（单曲项）：
  - 显示歌曲封面、名称、艺术家。
  - 鼠标悬停高亮，点击时发出 `ChooseMusic(int id)` 信号。

- `mediaplayerpool.h / mediaplayerpool.cpp`  
  使用小型对象池模式管理多个 `QMediaPlayer`，异步读取音频文件的元数据：
  - 解析标题、艺术家、封面图等。
  - 解析完成后通过 `taskFinished` 信号返回结果，更新对应 `SongUnit`。

- `lrcparser.h / lrcparser.cpp`  
  歌词解析器：
  - 支持多编码自动尝试。
  - 使用正则表达式解析 `[mm:ss.xx]` 或 `[mm:ss.xxx]` 格式的时间戳。
  - 提供按时间查找当前歌词行的接口。

- `marqueelabel.h / marqueelabel.cpp`  
  跑马灯标签：
  - 当文本宽度超过标签宽度时，自动横向循环滚动。
  - 用于主界面的歌名、艺术家名显示。

- `res.qrc`  
  Qt 资源文件，包含图标、背景图片等（如播放按钮图标、默认封面图片等）。

- `CMakeLists.txt`  
  CMake 构建配置，启用了 `AUTOUIC/AUTOMOC/AUTORCC`，并链接 `Qt6::Widgets` 和 `Qt6::Multimedia`。

---

## 环境与依赖

- **Qt 版本**：Qt 6.8.3（使用 Qt 6 的 `QMediaPlayer/QAudioOutput` API）
- **构建系统**：CMake（最低版本 3.16）
- **编译器**：支持 C++17 的编译器（例如 MSVC、Clang、GCC 等）

---

## 编译与运行

1. **准备音乐文件**

   - 在构建生成的可执行文件同级目录下创建 `MusicList` 文件夹，例如：
     - Windows：`<build-dir>\MusicPlayer.exe` 旁边创建 `MusicList\`
   - 将你的音频文件放到 `MusicList` 中（支持的格式见上）。
   - 如需歌词，在同一目录下放置同名 `.lrc` 文件，例如：
     - `song.mp3` 对应 `song.lrc`

2. **使用 CMake 构建**

   ```bash
   # 在项目根目录（包含 CMakeLists.txt）下
   mkdir build
   cd build

   cmake -G "Ninja" ..
   cmake --build .