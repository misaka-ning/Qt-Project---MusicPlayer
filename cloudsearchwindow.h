#pragma once

#include <QDialog>
#include <QVector>

#include "cloudmusicservice.h"

class QListWidget;
class QLabel;
class QPushButton;

class CloudSearchWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CloudSearchWindow(QWidget *parent = nullptr);

    void setResults(const QVector<CloudMusicService::CloudSongBrief>& songs);

signals:
    void songActivated(const QString& songId);

private:
    void activateCurrent();

private:
    QVector<CloudMusicService::CloudSongBrief> m_songs;
    QListWidget *m_list{nullptr};
    QLabel *m_hint{nullptr};
    QPushButton *m_playButton{nullptr};
};

