#ifndef SONGUNIT_H
#define SONGUNIT_H

#include <QWidget>
#include <QString>
#include <QUrl>
#include <QPixmap>
#include <QMouseEvent>

namespace Ui {
class SongUnit;
}

class SongUnit : public QWidget
{
    Q_OBJECT

public:
    explicit SongUnit(int id, QPixmap pix, QUrl url, QString name = "未知曲目", QString artist = "未知艺术家", QWidget *parent = nullptr);
    ~SongUnit();

    void SetPixmap(const QPixmap pix);
    void SetName(const QString name);
    void SetArtist(const QString artist);
    QUrl Geturl();
    void UiUpdate();

private:
    Ui::SongUnit *ui;

    int m_music_id;
    QUrl m_music_url;
    QString m_music_name;
    QString m_music_artist;
    QPixmap m_music_pixmap;

    void InitUnit();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void ChooseMusic(int id);

};

#endif // SONGUNIT_H
