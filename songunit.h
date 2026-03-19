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
    void SetId(int id);
    void SetPixmap(const QPixmap pix);
    void SetName(const QString name);
    void SetArtist(const QString artist);
    QUrl Geturl();
    void UiUpdate();              // 将当前封面/名称/艺术家刷新到 UI

private:
    Ui::SongUnit *ui;

    int m_music_id;
    QUrl m_music_url;
    QString m_music_name;
    QString m_music_artist;
    QPixmap m_music_pixmap;

    void InitUnit();              // 设置尺寸策略、样式与子控件初始内容

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void ChooseMusic(int id);     // 点击该单元时发射，参数为列表索引

};

#endif // SONGUNIT_H
