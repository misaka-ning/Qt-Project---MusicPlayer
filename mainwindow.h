#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVector>
#include <QQueue>
#include <QMap>
#include <QUrl>
#include "musicplaylist.h"
#include "mediaplayerpool.h"
#include "LrcParser.h"  // 包含歌词解析类头文件

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

enum nextmode{
    List_Play,   // 列表循环
    Loop_Play,   // 随机播放
    Repeat_Play  // 单曲循环
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    // QVector<QUrl> m_playlist;
    int m_playnum;
    MusicPlaylist *m_musicplaylist;
    QMap<QUrl, int> m_urlToIndex;   // 读取音频映射
    MediaPlayerPool *m_pool;
    bool m_autoplay;
    nextmode m_nextmode;
    QVector<int> m_randommap;     // 随机播放的唯一映射
    QList<int> m_shuffleOrder;    // 随机播放顺序列表（存储歌曲索引）
    int        m_shuffleIndex;    // 当前在随机顺序列表中的位置

    LrcParser *m_lrcParser;
    QVector<LrcLine> m_lyrics;
    void loadLyrics(const QString &musicFilePath);  // 加载歌词
    void updateLyrics(qint64 position);             // 更新歌词显示

    void InitWindow();
    void InitPool();
    void InitButtons();
    void InitButtonIcon(QPushButton *button, const QString &path);
    void InitPlayList();
    void InitPlay();
    void InitLrcParser();

    void PlayPrevSong();
    void PlayNextSong();
    void UpdateMusicListPosition();
    void UpdateRandomArray();
    void PlaySong();

private slots:
    void StatusChanged(QMediaPlayer::MediaStatus status);
    void StateChange(QMediaPlayer::PlaybackState state);
    void UpdateMetadata();
    void MusicEnd();

    void updateSliderPosition(qint64 position);
    void updateSliderRange(qint64 duration);
    void updatalyricsListWidget();
    void onProgressSliderMoved(int value);

    void onPositionChanged(qint64 position);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

};
#endif // MAINWINDOW_H
