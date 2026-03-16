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
#include <QPointF>
#include <QTimer>
#include <QModelIndex>
#include <QResizeEvent>
#include "musicplaylist.h"
#include "playercontroller.h"
#include "LrcParser.h"  // 包含歌词解析类头文件

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    MusicPlaylist *m_musicplaylist;
    PlayerController *m_playerController;

    bool  m_sliderPressed;       // 当前是否在按住进度条拖动
    bool  m_ignoreSliderUpdate;  // 在一次主动 seek 之后，短时间内忽略播放器发来的位置更新（防止“回弹”）
    qint64 m_pendingSeek;        // 如果在媒体还没加载完时进行拖动，记录一个待应用的进度值

    bool m_isDragging;           // 是否正在拖动窗口
    QPointF m_dragStartPos;       // 拖动开始的位置

    // 窗口调整大小相关变量
    enum Edge {
        NoEdge = 0,
        LeftEdge = 1,
        RightEdge = 2,
        TopEdge = 4,
        BottomEdge = 8,
        TopLeftEdge = TopEdge | LeftEdge,
        TopRightEdge = TopEdge | RightEdge,
        BottomLeftEdge = BottomEdge | LeftEdge,
        BottomRightEdge = BottomEdge | RightEdge
    };

    Edge m_resizeEdge;           // 当前调整大小的边缘
    bool m_isResizing;           // 是否正在调整大小
    QPoint m_resizeStartPos;     // 调整大小开始时的鼠标位置
    QRect m_resizeStartGeometry; // 调整大小开始时的窗口几何形状
    const int m_resizeBorderWidth = 5; // 调整大小边框宽度

    LrcParser *m_lrcParser;
    QVector<LrcLine> m_lyrics;
    QTimer *m_wheelTimer;         // 用于检测鼠标滚轮操作后的定时器
    bool m_manualScroll;          // 是否手动滚动了歌词
    void loadLyrics(const QString &musicFilePath);  // 加载歌词
    void updateLyrics(qint64 position);             // 更新歌词显示

    void InitWindow();
    void loadStyleSheet();
    void InitButtons();
    void InitButtonIcon(QPushButton *button, const QString &path);
    void InitPlayList();
    void InitLrcParser();

    void UpdateMusicListPosition();

    // 窗口调整大小相关函数
    Edge getEdge(const QPoint &pos);  // 根据鼠标位置获取边缘类型
    void updateCursor(Edge edge);     // 根据边缘类型更新鼠标光标
    void startResize(Edge edge, const QPoint &pos); // 开始调整大小
    void performResize(const QPoint &pos); // 执行调整大小

private slots:

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
    void onLyricsListWidgetClicked(QModelIndex index);
    void onWheelTimerTimeout();

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

};
#endif // MAINWINDOW_H
