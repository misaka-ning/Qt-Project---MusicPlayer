#ifndef MUSICPLAYLIST_H
#define MUSICPLAYLIST_H

#include <QWidget>
#include <QVector>
#include <QUrl>
#include <QVBoxLayout>
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
    bool isempty();
    QUrl Geturl(const int n);
    int Getsize();
    void updateItem(int idx, QPixmap image, QString name, QString artist);


private:
    Ui::MusicPlaylist *ui;

    QVector<SongUnit*> m_musiclist;
    QVBoxLayout *m_layout;

signals:
    void ChooseMusicpass(int id);

};

#endif // MUSICPLAYLIST_H
