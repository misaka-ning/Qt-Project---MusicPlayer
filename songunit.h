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
    explicit SongUnit(int id, QPixmap pix, QUrl url, QString name = "Unknown Title",
                      QString artist = "Unknown Artist", QString cloudSongId = QString(), QWidget *parent = nullptr);
    ~SongUnit();
    void SetId(int id);
    void SetPixmap(const QPixmap pix);
    void SetName(const QString name);
    void SetArtist(const QString artist);
    void SetUrl(const QUrl& url);
    void SetCloudSongId(const QString& cloudSongId);
    QUrl Geturl() const;
    QString GetCloudSongId() const;
    QString GetName() const;
    QString GetArtist() const;
    QPixmap GetPixmap() const;
    void UiUpdate();              // 将当前封面/名称/艺术家刷新到 UI

private:
    Ui::SongUnit *ui;

    int m_music_id;
    QUrl m_music_url;
    QString m_music_name;
    QString m_music_artist;
    QString m_cloud_song_id;
    QPixmap m_music_pixmap;

    void InitUnit();              // 设置尺寸策略、样式与子控件初始内容

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void ChooseMusic(int id);     // 点击该单元时发射，参数为列表索引

};

#endif // SONGUNIT_H
