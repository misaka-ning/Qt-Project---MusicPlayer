#include "playercontroller.h"

#include <QCoreApplication>
#include <QDir>
#include <QRandomGenerator>
#include <QTimer>

/**
 * @brief PlayerController 构造函数
 *
 * 这里只做播放器相关对象的基础创建与绑定；
 * 播放列表和文件扫描在 InitPlayList 中延迟完成，以避免构造阶段做太多工作。
 */
PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playnum(0)
    , m_musicplaylist(nullptr)
    , m_pool(nullptr)
    , m_autoplay(false)
    , m_nextmode(List_Play)
    , m_shuffleIndex(0)
    , m_playToken(0)
{
    m_player->setAudioOutput(m_audioOutput);

    // 部分音频在 play() 后会停留在 0ms 不前进（直到发生一次 seek），这里做一次“卡住检测”自动唤醒。
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state != QMediaPlayer::PlayingState) return;

        const int token = ++m_playToken;
        QTimer::singleShot(200, this, [this, token]() {
            if (token != m_playToken) return; // 已经切换了歌曲/状态，丢弃
            if (m_player->playbackState() != QMediaPlayer::PlayingState) return;
            if (m_player->mediaStatus() != QMediaPlayer::LoadedMedia &&
                m_player->mediaStatus() != QMediaPlayer::BufferedMedia &&
                m_player->mediaStatus() != QMediaPlayer::StalledMedia)
            {
                return;
            }

            // 如果已经开始前进就不处理；否则轻微 seek 1ms 触发解码/时钟启动
            if (m_player->position() == 0 && m_player->duration() > 0) {
                m_player->setPosition(1);
            }
        });
    });
}

/**
 * @brief 初始化媒体元数据解析对象池
 *
 * - 创建 MediaPlayerPool，并限制最大并发数（当前为 4）
 * - 将 taskFinished / taskFailed 信号转发为对 MusicPlaylist 的 UI 更新与调试输出
 */
void PlayerController::InitPool()
{
    // 创建对象池（最大并发数设为4）
    m_pool = new MediaPlayerPool(4, this);

    // 连接任务完成信号：更新播放列表对应项
    connect(m_pool, &MediaPlayerPool::taskFinished, this, [this](int taskId, const QPixmap &cover, const QString &title, const QString &artist) {
        if (!m_musicplaylist) return;
        if (taskId >= 0 && taskId < m_musicplaylist->Getsize()) {
            QPixmap finalCover = cover;
            if (finalCover.isNull()) {
                finalCover = QPixmap(":/res/misaka.png");
            }
            m_musicplaylist->updateItem(taskId, finalCover, title, artist);
        }
    });

    connect(m_pool, &MediaPlayerPool::taskFailed, this, [this](int taskId, const QString &error) {
        Q_UNUSED(taskId);
        qDebug() << "任务失败:" << error;
    });
}

/**
 * @brief 初始化播放列表，并与外部的 MusicPlaylist 视图绑定
 *
 * 该函数会：
 *  - 记录外部传入的 MusicPlaylist 指针（不负责释放）
 *  - 确保 MediaPlayerPool 已经初始化
 *  - 扫描应用目录下的 MusicList 文件夹，以支持的后缀过滤音频文件
 *  - 向 MusicPlaylist 追加 SongUnit 占位项，并为每个文件提交一个元数据解析任务
 *  - 在存在歌曲时，为 QMediaPlayer 设置初始播放源和音量
 */
void PlayerController::InitPlayList(MusicPlaylist *playlist)
{
    m_musicplaylist = playlist;
    if (!m_musicplaylist) {
        emit playlistAvailabilityChanged(false);
        return;
    }

    if (!m_pool) {
        InitPool();
    }

    // 在文件同路径中找到 MusicList 文件夹
    QString exeDir = QCoreApplication::applicationDirPath();
    QString musicListPath = exeDir + "/MusicList";
    QDir dir;

    if (!dir.exists(musicListPath)) {
        dir.mkdir(musicListPath);
    }

    // 定义支持的音频格式（只加载这些类型的文件）
    QStringList audioSuffixes = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};

    QDir musicDir(musicListPath);
    QFileInfoList allFiles = musicDir.entryInfoList(QDir::Files);
    int index = 0;

    foreach (const QFileInfo &fileInfo, allFiles) {
        QString suffix = fileInfo.suffix().toLower();
        if (!audioSuffixes.contains(suffix))
            continue;

        QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());

        m_musicplaylist->AppendMusic(QPixmap(":/res/misaka.png"), url, "加载中", "加载中");
        m_urlToIndex[url] = index;
        if (m_pool) {
            m_pool->addTask(url, index);
        }

        ++index;
    }

    if (m_pool) {
        m_pool->start();
    }

    m_shuffleOrder.clear();
    m_shuffleIndex = 0;

    if (!m_musicplaylist->isempty()) {
        ensureValidPlayIndex();
        m_player->setSource(m_musicplaylist->Geturl(m_playnum));
        m_audioOutput->setVolume(1);
    }

    emit playlistAvailabilityChanged(!m_musicplaylist->isempty());
}

/**
 * @brief 生成/刷新随机播放顺序数组
 *
 * 只在 Loop_Play（随机播放）模式下使用。
 * 通过 Fisher-Yates 洗牌算法生成 0~size-1 的随机排列，
 * 并同步当前 m_playnum 在随机列表中的位置到 m_shuffleIndex。
 */
void PlayerController::UpdateRandomArray()
{
    if (!m_musicplaylist) return;

    int size = m_musicplaylist->Getsize();
    m_shuffleOrder.clear();
    if (size == 0) return;

    ensureValidPlayIndex();
    for (int i = 0; i < size; ++i) {
        m_shuffleOrder.append(i);
    }

    for (int i = size - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        m_shuffleOrder.swapItemsAt(i, j);
    }

    m_shuffleIndex = m_shuffleOrder.indexOf(m_playnum);
    if (m_shuffleIndex < 0) m_shuffleIndex = 0;
}

bool PlayerController::ensureValidPlayIndex()
{
    if (!m_musicplaylist) return false;
    const int size = m_musicplaylist->Getsize();
    if (size <= 0) return false;

    if (m_playnum < 0) m_playnum = 0;
    if (m_playnum >= size) m_playnum = size - 1;
    return true;
}

/**
 * @brief 根据当前 m_playnum 播放对应的歌曲
 *
 * 只负责设置播放源并在需要时自动调用 play()，
 * 不改变 m_playnum 值本身。
 */
void PlayerController::PlaySong()
{
    if (!m_musicplaylist || m_musicplaylist->isempty()) return;
    if (!ensureValidPlayIndex()) return;

    m_player->setSource(m_musicplaylist->Geturl(m_playnum));

    if (m_autoplay)
    {
        QTimer::singleShot(500, this, [this]() {
            m_autoplay = false;
            m_player->play();
        });
    }
}

/**
 * @brief 切换到上一首歌曲
 *
 * 根据当前播放模式执行不同的索引更新策略：
 *  - List_Play   : 按顺序向前移动（支持从第一首跳到最后一首）
 *  - Loop_Play   : 在洗牌序列中向前移动一位
 *  - Repeat_Play : 不切换歌曲，只是将进度条回到开头位置
 */
void PlayerController::PlayPrevSong()
{
    if (!m_musicplaylist || m_musicplaylist->isempty()) return;
    if (!ensureValidPlayIndex()) return;
    if (m_player->isPlaying()) m_autoplay = true;

    // 单曲循环不需要换源文件
    if(m_nextmode == Repeat_Play)
    {
        m_player->setPosition(1);
        m_player->play();
        return;
    }

    if (m_nextmode == List_Play)
    {
        m_playnum--;
        if (m_playnum == -1) m_playnum = m_musicplaylist->Getsize() - 1;
    }
    else if (m_nextmode == Loop_Play)
    {
        if (m_shuffleOrder.isEmpty()) {
            UpdateRandomArray();
        }
        if (!m_shuffleOrder.isEmpty()) {
            m_shuffleIndex = (m_shuffleIndex - 1 + m_shuffleOrder.size()) % m_shuffleOrder.size();
            m_playnum = m_shuffleOrder[m_shuffleIndex];
        }
    }

    PlaySong();
}

/**
 * @brief 切换到下一首歌曲
 *
 * 根据当前播放模式执行不同的索引更新策略：
 *  - List_Play   : 按顺序向后移动（尾首相连）
 *  - Loop_Play   : 在洗牌序列中向后移动一位
 *  - Repeat_Play : 不切换歌曲，只是将进度条回到开头位置
 */
void PlayerController::PlayNextSong()
{
    if (!m_musicplaylist || m_musicplaylist->isempty()) return;
    if (!ensureValidPlayIndex()) return;
    if (m_player->isPlaying()) m_autoplay = true;

    // 单曲循环不需要换源文件
    if(m_nextmode == Repeat_Play)
    {
        m_player->setPosition(1);
        m_player->play();
        return;
    }

    if (m_nextmode == List_Play)
    {
        m_playnum = (m_playnum + 1) % m_musicplaylist->Getsize();
    }
    else if (m_nextmode == Loop_Play)
    {
        if (m_shuffleOrder.isEmpty()) {
            UpdateRandomArray();
        }
        if (!m_shuffleOrder.isEmpty()) {
            m_shuffleIndex = (m_shuffleIndex + 1) % m_shuffleOrder.size();
            m_playnum = m_shuffleOrder[m_shuffleIndex];
        }
    }

    PlaySong();
}

/**
 * @brief 播放结束时的统一处理函数
 *
 * 一般由 MainWindow 在进度条走到末尾时调用，
 * 内部通过设置 m_autoplay 标志并调用 PlayNextSong() 来实现“自动下一首”。
 */
void PlayerController::MusicEnd()
{
    if (m_musicplaylist && !m_musicplaylist->isempty()) {
        PlayNextSong();
    }
}

/**
 * @brief 设置当前播放模式
 *
 * 切换到 Loop_Play 时会自动生成新的随机队列；
 * 切换回 List_Play 时会清空随机队列。
 */
void PlayerController::SetPlayMode(nextmode mode)
{
    m_nextmode = mode;
    if (m_nextmode == Loop_Play) {
        UpdateRandomArray();
    } else if (m_nextmode == List_Play) {
        m_shuffleOrder.clear();
    }
}

/**
 * @brief 响应 MusicPlaylist 的选中信号，切换到指定歌曲
 *
 * @param id 在 MusicPlaylist 中的歌曲索引
 *
 * 在 Loop_Play 模式下会确保随机队列中存在对应索引，并同步 m_shuffleIndex；
 * 其他模式下仅简单设置 m_playnum。
 */
void PlayerController::OnChooseMusic(int id)
{
    if (!m_musicplaylist || m_musicplaylist->isempty()) return;
    const int size = m_musicplaylist->Getsize();
    if (id < 0 || id >= size) return;

    if (m_nextmode == Loop_Play)
    {
        m_playnum = id;
        if (m_shuffleOrder.isEmpty()) {
            UpdateRandomArray();
        }
        int idx = m_shuffleOrder.indexOf(m_playnum);
        if (idx == -1) {
            UpdateRandomArray();
        } else {
            m_shuffleIndex = idx;
        }
    }
    else
    {
        m_playnum = id;
    }
    if (m_player->isPlaying()) m_autoplay = true;
    PlaySong();
}

