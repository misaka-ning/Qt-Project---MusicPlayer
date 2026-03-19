#ifndef MUSICPLAYLIST_H
#define MUSICPLAYLIST_H

#include <QWidget>
#include <QVector>
#include <QUrl>
#include <QVBoxLayout>
#include <QPoint>
#include <QLabel>
#include "songunit.h"

namespace Ui {
class MusicPlaylist;
}

class MusicPlaylist : public QWidget
{
    Q_OBJECT

public:
    explicit MusicPlaylist(QWidget *parent = nullptr);
    ~MusicPlaylist();

    void AppendMusic(QPixmap pix, QUrl url, QString name, QString artist);  // 直接追加一首，无返回值
    int appendSong(const QPixmap& pix, const QUrl& url, const QString& name, const QString& artist);  // 返回索引的便捷接口
    bool removeSongAt(int index);
    void clearSongs();
    bool isempty();
    QUrl Geturl(const int n);
    int Getsize();
    void updateItem(int idx, QPixmap image, QString name, QString artist);
    bool hasSongs() const { return !m_musiclist.isEmpty(); }
    void setTargetPos(const QPoint& p);
    QPoint targetPos() const;
    void showAnimated();
    void hideAnimated();
    bool isAnimating() const;

private:
    Ui::MusicPlaylist *ui;
    QVector<SongUnit*> m_musiclist;
    QVBoxLayout *m_layout;
    QLabel *m_emptyLabel{nullptr};
    QPoint m_targetPos{0, 0};
    class QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    class QPropertyAnimation* m_opacityAnim{nullptr};
    class QPropertyAnimation* m_posAnim{nullptr};
    class QParallelAnimationGroup* m_animGroup{nullptr};
    bool m_isAnimating{false};
    bool m_shouldBeVisible{false};

    int slideOffsetPx() const;        // 滑入/滑出偏移像素
    int animDurationMs() const;      // 动画时长（毫秒）
    void updateEmptyStateUi();       // 根据列表是否为空更新占位标签

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void ChooseMusicpass(int id);
    void songsChanged(int size);
    void hasSongsChanged(bool hasSongs);
};

#endif // MUSICPLAYLIST_H
