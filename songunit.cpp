#include "songunit.h"
#include "ui_songunit.h"

#include <QLabel>

void SongUnit::InitUnit()
{
    // setFixedSize(400, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(80);  // 保持最小高度
    setStyleSheet("background-color: White;");
    setAttribute(Qt::WA_StyledBackground, true);

    ui->imagelabel->setScaledContents(true);
    ui->imagelabel->setPixmap(m_music_pixmap);
    ui->namelabel->setText(m_music_name);
    ui->artistlabel->setText(m_music_artist);
}

SongUnit::SongUnit(QPixmap pix, QUrl url, QString name, QString artist, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SongUnit),m_music_pixmap(pix) ,m_music_url(url), m_music_name(name), m_music_artist(artist)
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
