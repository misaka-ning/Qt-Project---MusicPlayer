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

    void AppendMusic(QPixmap pix, QUrl url, QString name, QString artist);

    // Mutation API (for dynamic playlist management)
    int appendSong(const QPixmap& pix, const QUrl& url, const QString& name, const QString& artist);
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

    int slideOffsetPx() const;
    int animDurationMs() const;
    void updateEmptyStateUi();

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void ChooseMusicpass(int id);
    void songsChanged(int size);
    void hasSongsChanged(bool hasSongs);

};

#endif // MUSICPLAYLIST_H
