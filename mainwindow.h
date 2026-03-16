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

    LrcParser *m_lrcParser;
    QVector<LrcLine> m_lyrics;
    void loadLyrics(const QString &musicFilePath);  // 加载歌词
    void updateLyrics(qint64 position);             // 更新歌词显示

    void InitWindow();
    void InitButtons();
    void InitButtonIcon(QPushButton *button, const QString &path);
    void InitPlayList();
    void InitLrcParser();

    void UpdateMusicListPosition();

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
