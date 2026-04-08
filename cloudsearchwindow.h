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

    // 刷新搜索结果列表（会覆盖旧数据）。
    void setResults(const QVector<CloudMusicService::CloudSongBrief>& songs);

signals:
    // 用户确认播放当前选中项时发出 songId。
    void songActivated(const QString& songId);

private:
    // 播放按钮/双击列表项共用的激活入口。
    void activateCurrent();

private:
    QVector<CloudMusicService::CloudSongBrief> m_songs;
    QListWidget *m_list{nullptr};
    QLabel *m_hint{nullptr};
    QPushButton *m_playButton{nullptr};
};

