#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMap>
#include <QUrl>
#include "musicplaylist.h"
#include "mediaplayerpool.h"

/**
 * @brief 播放模式枚举（保持原有命名风格）
 * List_Play  : 按顺序循环整张列表
 * Loop_Play  : 开启随机播放（列表乱序循环）
 * Repeat_Play: 单曲循环当前歌曲
 */
enum nextmode{
    List_Play,   // 列表循环
    Loop_Play,   // 随机播放
    Repeat_Play  // 单曲循环
};

/**
 * @brief PlayerController 负责管理音乐播放相关的核心逻辑。
 *
 * 主要职责：
 *  - 持有并管理 QMediaPlayer / QAudioOutput 实例
 *  - 初始化并维护 MusicPlaylist（扫描 MusicList 目录、提交元数据解析任务）
 *  - 处理上一首 / 下一首 / 单曲循环 / 随机播放等播放模式逻辑
 *  - 提供简单接口给 MainWindow 调用（UI 只关心“想要做什么”，不关心怎么做）
 */
class PlayerController : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent Qt 对象树中的父对象
     *
     * 在构造函数中只完成播放器的基本初始化（创建 QMediaPlayer/QAudioOutput，设置输出），
     * 不做任何磁盘扫描和播放列表构建，避免阻塞 UI。
     */
    explicit PlayerController(QObject *parent = nullptr);

    /**
     * @brief 获取内部使用的 QMediaPlayer 指针
     *
     * 提供给 MainWindow 用于连接信号槽（比如 positionChanged、durationChanged 等），
     * 不建议在外部直接修改其播放列表和源。
     */
    QMediaPlayer *GetPlayer() const { return m_player; }

    /**
     * @brief 获取内部使用的 QAudioOutput 指针
     *
     * 可以用于在 UI 中控制音量、静音等属性。
     */
    QAudioOutput *GetAudioOutput() const { return m_audioOutput; }

    /**
     * @brief 初始化播放列表数据，并绑定到指定的 MusicPlaylist 视图上。
     *
     * - 负责扫描可执行文件同目录下的 MusicList 文件夹
     * - 为每个音频文件在 MusicPlaylist 中追加一个 SongUnit
     * - 利用 MediaPlayerPool 异步解析封面、标题、艺术家等元数据
     * - 设置当前播放器的初始播放源（第一首歌）
     */
    void InitPlayList(MusicPlaylist *playlist);

    /**
     * @brief 切换到上一首歌（根据当前播放模式决定行为）
     */
    void PlayPrevSong();

    /**
     * @brief 切换到下一首歌（根据当前播放模式决定行为）
     */
    void PlayNextSong();

    /**
     * @brief 触发当前索引对应歌曲的播放
     *
     * 该函数只负责根据 m_playnum 设置播放器源并在需要时自动 play，
     * 不改变 m_playnum 的值。
     */
    void PlaySong();

    /**
     * @brief 播放结束时的统一处理入口
     *
     * 主要用于“自动下一首”逻辑，供 MainWindow 在进度条到达末尾时调用。
     */
    void MusicEnd();

    /**
     * @brief 设置当前播放模式
     *
     * 会同步维护随机播放所需的 m_shuffleOrder / m_shuffleIndex。
     */
    void SetPlayMode(nextmode mode);

    /**
     * @brief 获取当前播放模式
     */
    nextmode GetPlayMode() const { return m_nextmode; }

    /**
     * @brief 获取当前正在播放的歌曲索引
     *
     * 对应 MusicPlaylist 中的下标，用于外部（例如 MainWindow）获取当前歌曲信息或歌词。
     */
    int GetCurrentIndex() const { return m_playnum; }

    /**
     * @brief 设置下一次换源时自动播放
     *
     * 当歌曲自动运行到结束时，isplay()为false，但仍有可能有需要自动播放的需求
     */
    void SetAutoPlay(bool flag) {m_autoplay = flag;}

signals:
    /**
     * @brief 播放列表是否可用（是否有歌曲）状态变化
     *
     * 用于通知 UI 启用/禁用播放相关控件与空列表占位状态。
     */
    void playlistAvailabilityChanged(bool hasSongs);

public slots:
    /**
     * @brief 由 MusicPlaylist 发出的“选中某首歌”信号触发
     * @param id 列表中的歌曲索引
     *
     * 根据当前播放模式更新内部索引并调用 PlaySong()。
     */
    void OnChooseMusic(int id);

private:
    // 播放核心
    QMediaPlayer *m_player;        ///< 实际执行解码和播放的播放器对象
    QAudioOutput *m_audioOutput;   ///< 音频输出设备（扬声器/耳机）

    // 播放列表相关
    int m_playnum;                         ///< 当前播放的歌曲索引（在 MusicPlaylist 中的位置）
    MusicPlaylist *m_musicplaylist;        ///< 播放列表 UI 组件（不负责其生命周期）
    QMap<QUrl, int> m_urlToIndex;          ///< 文件 URL 到索引的映射，便于后续扩展

    // 异步元数据加载
    MediaPlayerPool *m_pool;               ///< 使用多个 QMediaPlayer 异步解析封面和标签

    // 播放状态
    bool m_autoplay;                       ///< 是否在切歌后自动播放（用于“自动下一首”场景）
    nextmode m_nextmode;                   ///< 当前播放模式
    QList<int> m_shuffleOrder;             ///< 随机播放顺序列表（存储歌曲索引）
    int        m_shuffleIndex;             ///< 当前在随机顺序列表中的位置
    int        m_playToken;                ///< 用于丢弃过期的延迟检查（防止多次播放切换时误触发）

    /**
     * @brief 初始化 MediaPlayerPool，并连接任务完成/失败信号。
     */
    void InitPool();

    /**
     * @brief 重新生成一组随机播放顺序，并同步 m_shuffleIndex。
     *
     * 仅在 Loop_Play（随机播放）模式下使用。
     */
    void UpdateRandomArray();

    /**
     * @brief 确保 m_playnum 始终落在 [0, size) 范围内
     *
     * 当播放列表为空时返回 false；非空时会对越界值进行 clamp，并返回 true。
     */
    bool ensureValidPlayIndex();
};

#endif // PLAYERCONTROLLER_H

