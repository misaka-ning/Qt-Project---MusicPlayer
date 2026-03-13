#include "musicplaylist.h"
#include "ui_musicplaylist.h"

MusicPlaylist::MusicPlaylist(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MusicPlaylist)
{
    ui->setupUi(this);

    // 禁止水平滚动
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 隐藏垂直滚动条（但保留垂直滚动功能，鼠标滚轮仍可滚动）
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                  "stop:0 #FEFEFE, stop:0.5 #FBFBFB, stop:1 #FEFEFE); "
                  "border-radius: 10px;");

    // 获取内部容器的布局
    m_layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!m_layout) {
        // 如果 UI 中未正确设置布局，则创建一个（安全后备）
        m_layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
    }
}

MusicPlaylist::~MusicPlaylist()
{
    delete ui;
}

void MusicPlaylist::AppendMusic(QPixmap pix, QUrl url, QString name, QString artist)
{
    SongUnit *unit = new SongUnit(pix, url, name, artist, this);
    // 添加到布局（插入到弹簧之前）
    if (m_layout->count() > 0) {
        // 如果布局中有弹簧，插入到最后一个元素（弹簧）之前，确保弹簧始终在最底部
        m_layout->insertWidget(m_layout->count() - 1, unit);
    } else {
        m_layout->addWidget(unit);
    }
    unit->show();

    m_musiclist.append(unit);
}

bool MusicPlaylist::isempty()
{
    return m_musiclist.empty();
}

QUrl MusicPlaylist::Geturl(const int n)
{
    return m_musiclist[n]->Geturl();
}

int MusicPlaylist::Getsize()
{
    return m_musiclist.size();
}

void MusicPlaylist::updateItem(int idx, QPixmap image, QString name, QString artist)
{
    m_musiclist[idx]->SetPixmap(image);
    m_musiclist[idx]->SetName(name);
    m_musiclist[idx]->SetArtist(artist);
    m_musiclist[idx]->UiUpdate();
}
