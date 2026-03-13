#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMediaMetaData>
#include <QDir>
#include <QCoreApplication>
#include <QResizeEvent>
#include "songunit.h"

// 初始化主窗口
void MainWindow::InitWindow()
{
    setWindowTitle("MusicPlayer");
    this->resize(1235, 833);
    ui->Controlwidget->setStyleSheet("background-color: White; border-radius: 10px;");
    this->setStyleSheet("MainWindow{border-image: url(:/res/background_fade.png) 0 0 0 0 stretch stretch;}");

    ui->imagelabel->setFixedSize(300, 300);
    ui->imagelabel->setScaledContents(true);

    m_playnum = 0;
}

void MainWindow::InitPool()
{
    // 创建对象池（最大并发数设为4）
    m_pool = new MediaPlayerPool(5, this);

    // 连接任务完成信号：更新播放列表对应项
    connect(m_pool, &MediaPlayerPool::taskFinished, this, [this](int taskId, const QPixmap &cover, const QString &title, const QString &artist) {
        if (taskId >= 0 && taskId < m_musicplaylist->Getsize()) {
            // 如果封面有效则使用，否则保持默认图片
            QPixmap finalCover = cover;
            if (finalCover.isNull()) {
                finalCover = QPixmap(":/res/misaka.png");
            }
            m_musicplaylist->updateItem(taskId, finalCover, title, artist);
        }
    });

    // 连接任务失败信号（可选，用于调试）
    connect(m_pool, &MediaPlayerPool::taskFailed, this, [this](int taskId, const QString &error) {
        qDebug() << "任务失败，索引:" << taskId << "错误:" << error;
    });
}

// 初始化所有按钮
void MainWindow::InitButtons()
{
    InitButtonIcon(ui->prevButton, ":/res/prev song.png");
    InitButtonIcon(ui->playButton, ":/res/play.png");
    InitButtonIcon(ui->nextButton, ":/res/next song.png");
    InitButtonIcon(ui->modeButton, ":/res/list play.png");
    InitButtonIcon(ui->listButton, ":/res/playlist.png");

    // 实现上一首按钮功能
    connect(ui->prevButton, &QPushButton::clicked, this, &MainWindow::PlayPrevSong);
    // 实现下一首按钮功能
    connect(ui->nextButton, &QPushButton::clicked, this, &MainWindow::PlayNextSong);

    // 实现使用playButton按钮控制音乐暂停、播放功能
    connect(ui->playButton, &QPushButton::clicked, this, [this](){
        if(m_player->isPlaying())
        {
            m_player->pause();
            InitButtonIcon(ui->playButton, ":/res/play.png");
        }
        else
        {
            m_player->play();
            InitButtonIcon(ui->playButton, ":/res/stop.png");
        }
    });

    // 实现点击listButton时，弹出或关闭音乐播放列表窗口
    connect(ui->listButton, &QPushButton::clicked, this, [this](){
        if(m_musicplaylist->isVisible())
        {
            m_musicplaylist->hide();
        }
        else
        {
            m_musicplaylist->show();
        }
    });
}

#if 0
// 初始化背景（弃用）
void MainWindow::setBackGround(const QString &path)
{

    QPixmap pixmap(path);
    QSize windowSize = this->size();
    QPixmap scalePixmap = pixmap.scaled(windowSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(scalePixmap));
    setAutoFillBackground(true);
    this->setPalette(palette);
}
#endif

// 初始化按钮图标
void MainWindow::InitButtonIcon(QPushButton *button, const QString & path)
{
    button->setFixedSize(30, 30);
    button->setIcon(QIcon(path));
    button->setIconSize(QSize(button->width(), button->height()));
    button->setStyleSheet("background-color:transparent; border-style:none; background-color: White;");
}

// 初始化播放列表
void MainWindow::InitPlayList()
{
    // 在文件同路径中找到MusicList文件夹
    QString exeDir = QCoreApplication::applicationDirPath();
    QString musicListPath = exeDir + "/MusicList";
    QDir dir;

    // 检查文件夹是否存在，如果不存在则创建
    if (!dir.exists(musicListPath)) {
        if (dir.mkdir(musicListPath)) {
            qDebug() << "MusicList 文件夹创建成功：" << musicListPath;
        } else {
            qDebug() << "错误：无法创建 MusicList 文件夹！";
        }
    } else {
        qDebug() << "MusicList 文件夹已存在：" << musicListPath;
    }

    // 创建播放列表
    // m_playlist.clear();
    m_musicplaylist = new MusicPlaylist(this);
    m_musicplaylist->hide();
    UpdateMusicListPosition();

    // 定义支持的音频格式
    QStringList audioSuffixes = {"mp3", "wav", "flac", "aac", "ogg", "m4a", "wma"};

    // 遍历 MusicList 文件夹，添加音频文件
    QDir musicDir(musicListPath);
    QFileInfoList allFiles = musicDir.entryInfoList(QDir::Files); // 获取所有文件
    int index = 0;

    foreach (const QFileInfo &fileInfo, allFiles) {
        QString suffix = fileInfo.suffix().toLower();
        if (!audioSuffixes.contains(suffix))
            continue;

        QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());

        // 1. 先用占位数据添加到播放列表
        m_musicplaylist->AppendMusic(QPixmap(":/res/misaka.png"), url, "加载中", "加载中");

        // 2. 记录URL到索引的映射（可选，但保留）
        m_urlToIndex[url] = index;

        // 3. 添加任务到对象池，使用索引作为任务ID
        m_pool->addTask(url, index);

        ++index;
    }

    // 4. 启动对象池处理任务
    m_pool->start();

    // 设置当前播放源为列表第一首（不自动播放）
    if (!m_musicplaylist->isempty()) {
        m_player->setSource(m_musicplaylist->Geturl(m_playnum));
        m_audioOutput->setVolume(1);
    }
}

// 初始化音乐播放器
void MainWindow::InitPlay()
{
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    m_player->setAudioOutput(m_audioOutput);
}

// 自动播放上一首
void MainWindow::PlayPrevSong()
{
    if (m_musicplaylist->isempty()) return;
    m_playnum--;
    if(m_playnum == -1) m_playnum = m_musicplaylist->Getsize() - 1;
    m_player->setSource(m_musicplaylist->Geturl(m_playnum));
}

// 自动播放下一首
void MainWindow::PlayNextSong()
{
    if (m_musicplaylist->isempty()) return;
    m_playnum = (m_playnum + 1) % m_musicplaylist->Getsize();
    m_player->setSource(m_musicplaylist->Geturl(m_playnum));
}

void MainWindow::UpdateMusicListPosition()
{
    int window_width = this->width();
    int window_height = this->height();
    int target_x = window_width - 390;
    int target_h = window_height - 300;
    m_musicplaylist->move(target_x, 100);
    m_musicplaylist->setFixedHeight(target_h);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 初始化
    InitWindow();
    InitPool();
    InitButtons();
    InitPlay();
    InitPlayList();

    // 音乐准备完毕信号槽
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::StatusChanged);

    // 音乐播放结束信号槽
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::StateChange);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 媒体状态处理
void MainWindow::StatusChanged(QMediaPlayer::MediaStatus status)
{
    // handleCursor(status);

    switch (status) {
    case QMediaPlayer::NoMedia:          // 无媒体
        break;
    case QMediaPlayer::LoadingMedia:     // 正在加载媒体
        break;
    case QMediaPlayer::LoadedMedia:      // 已加载媒体
        UpdateMetadata();
        break;
    case QMediaPlayer::BufferingMedia:   // 正在缓冲媒体
        break;
    case QMediaPlayer::BufferedMedia:    // 缓冲完成，可播放
        break;
    case QMediaPlayer::StalledMedia:     // 播放停滞（缓冲不足）
        break;
    case QMediaPlayer::EndOfMedia:       // 已到达媒体末尾
        break;
    case QMediaPlayer::InvalidMedia:     // 无效媒体
        break;
    }
}

// 更新音乐元数据
void MainWindow::UpdateMetadata()
{
    QMediaMetaData metaData = m_player->metaData();

    MarqueeLabel *artistlabel = ui->Controlwidget->findChild<MarqueeLabel*>("artistlabel");
    MarqueeLabel *namelabel = ui->Controlwidget->findChild<MarqueeLabel*>("namelabel");

    bool image_flag = true;

    // 设置默认文本
    if (namelabel) namelabel->setText("未知曲目");
    if (artistlabel) artistlabel->setText("未知艺术家");

    for (auto &&[key, value] : metaData.asKeyValueRange())
    {
        if(key == QMediaMetaData::ThumbnailImage)
        {
            image_flag = false;
            ui->imagelabel->setPixmap(QPixmap::fromImage(value.value<QImage>()));
        }
        else if(key == QMediaMetaData::ContributingArtist)
        {
            if (artistlabel) artistlabel->setText(value.toString());
        }
        else if(key == QMediaMetaData::Title)
        {
            if (namelabel) namelabel->setText(value.toString());
        }
    }

    if(image_flag) ui->imagelabel->setPixmap(QPixmap(":/res/misaka.png"));

    // 解决一些莫名其妙的bug
    m_player->setPosition(1);
}

void MainWindow::StateChange(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        MusicEnd();
        break;
    case QMediaPlayer::PlayingState:

        break;
    case QMediaPlayer::PausedState:

        break;
    }
}

void MainWindow::MusicEnd()
{
    InitButtonIcon(ui->playButton, ":/res/play.png");
    // 自动下一首
    if (!m_musicplaylist->isempty()) PlayNextSong();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    UpdateMusicListPosition();
}
