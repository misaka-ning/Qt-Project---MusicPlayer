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

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    // QVector<QUrl> m_playlist;
    int m_playnum;
    MusicPlaylist *m_musicplaylist;
    QMap<QUrl, int> m_urlToIndex;
    MediaPlayerPool *m_pool;

    void InitWindow();
    void InitPool();
    void InitButtons();
    void InitButtonIcon(QPushButton *button, const QString &path);
    void InitPlayList();
    void InitPlay();

    void PlayPrevSong();
    void PlayNextSong();
    void UpdateMusicListPosition();

private slots:
    void StatusChanged(QMediaPlayer::MediaStatus status);
    void StateChange(QMediaPlayer::PlaybackState state);
    void UpdateMetadata();
    void MusicEnd();

protected:
    void resizeEvent(QResizeEvent *event) override;

};
#endif // MAINWINDOW_H
