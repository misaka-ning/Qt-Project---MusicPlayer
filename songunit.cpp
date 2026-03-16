#include "songunit.h"
#include "ui_songunit.h"

#include <QLabel>

void SongUnit::InitUnit()
{
    // setFixedSize(400, 80);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(80);  // 保持最小高度
    setAttribute(Qt::WA_StyledBackground, true);

    ui->imagelabel->setScaledContents(true);
    ui->imagelabel->setPixmap(m_music_pixmap);
    ui->namelabel->setText(m_music_name);
    ui->artistlabel->setText(m_music_artist);
}

SongUnit::SongUnit(int id, QPixmap pix, QUrl url, QString name, QString artist, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SongUnit), m_music_id(id), m_music_pixmap(pix) ,m_music_url(url), m_music_name(name), m_music_artist(artist)
{
    ui->setupUi(this);
    InitUnit();
}


SongUnit::~SongUnit()
{
    delete ui;
}

void SongUnit::SetPixmap(const QPixmap pix)
{
    m_music_pixmap = pix;
}

void SongUnit::SetName(const QString name)
{
    m_music_name = name;
}

void SongUnit::SetArtist(const QString artist)
{
    m_music_artist = artist;
}

QUrl SongUnit::Geturl()
{
    return m_music_url;
}

void SongUnit::UiUpdate()
{
    ui->imagelabel->setPixmap(m_music_pixmap);
    ui->namelabel->setText(m_music_name);
    ui->artistlabel->setText(m_music_artist);
}

// 鼠标进入SongUnit时触发
void SongUnit::enterEvent(QEnterEvent *event)
{
    setStyleSheet("SongUnit { background-color: lightgray; }");
    ui->namelabel->setStyleSheet("background-color: lightgray");
    ui->artistlabel->setStyleSheet("background-color: lightgray");

    // 样式已在QSS文件中定义，无需在这里设置
    QWidget::enterEvent(event);
}

// 鼠标离开SongUnit时触发
void SongUnit::leaveEvent(QEvent *event)
{
    setStyleSheet("SongUnit { background-color: white; }");
    ui->namelabel->setStyleSheet("background-color: white; ");
    ui->artistlabel->setStyleSheet("background-color: white; ");

    // 样式已在QSS文件中定义，无需在这里设置
    QWidget::leaveEvent(event);
}

void SongUnit::mouseReleaseEvent(QMouseEvent *event)
{
    emit ChooseMusic(m_music_id);
    QWidget::mouseReleaseEvent(event);
}

