#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMediaMetaData>
#include <QDir>
#include <QCoreApplication>
#include <QResizeEvent>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QMouseEvent>
#include <QStyle>
#include "songunit.h"

// 初始化主窗口
void MainWindow::InitWindow()
{
    setWindowTitle("MusicPlayer");
    this->resize(1235, 833);
    ui->Controlwidget->setStyleSheet("background-color: White; border-radius: 10px;");
    this->setStyleSheet("MainWindow{border-image: url(:/res/background_fade2.png) 0 0 0 0 stretch stretch;}");

    ui->imagelabel->setFixedSize(300, 300);
    ui->imagelabel->setScaledContents(true);

    m_playnum = 0;
    m_autoplay = false;
    m_nextmode = List_Play;

    ui->Slider->installEventFilter(this);
}

void MainWindow::InitPool()
{
    // 创建对象池（最大并发数设为4）
    m_pool = new MediaPlayerPool(4, this);

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

    // 实现循环模式按钮功能
    connect(ui->modeButton, &QPushButton::clicked, this, [this](){
        switch(m_nextmode)
        {
            case(List_Play):
            {
                m_nextmode = Repeat_Play;
                InitButtonIcon(ui->modeButton, ":/res/repeat play.png");
            }
            break;
            case(Loop_Play):
            {
                m_nextmode = List_Play;
                InitButtonIcon(ui->modeButton, ":/res/list play.png");
                m_shuffleOrder.clear();
            }
            break;
            case(Repeat_Play):
            {
                m_nextmode = Loop_Play;
                InitButtonIcon(ui->modeButton, ":/res/loop play.png");
                // 生成随机播放顺序
                UpdateRandomArray();
            }
            break;
        }
    });

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

    // 初始化洗牌顺序为空（当前为列表模式，暂不需要）
    m_shuffleOrder.clear();
    m_shuffleIndex = 0;

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

// 初始化播放列表
void MainWindow::InitLrcParser()
{
    // 歌词解析对象
    m_lrcParser = new LrcParser(this);
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);

    ui->lyricsListWidget->setStyleSheet(R"(
    QListWidget {
        background-color: transparent;          /* 背景透明 */
        border: none;                           /* 无边框 */
        outline: none;                           /* 移除焦点虚线 */
        font-family: "Microsoft YaHei", "PingFang SC", sans-serif;
        font-size: 21px;
        color: rgba(255, 255, 255, 225);         /* 半透明白色文字 */
    }
    QListWidget::item {
        padding: 8px 15px;
        border-bottom: 1px solid rgba(255, 255, 255, 30); /* 半透明分割线 */
        background-color: transparent;
    }
    QListWidget::item:hover {
        background-color: rgba(255, 255, 255, 30); /* 悬停效果保留，提升交互 */
        color: white;
    }
    QListWidget::item:selected {
        background-color: rgba(218, 218, 218, 100);
        color: white;
    }
)");

    ui->songnamelabel->setStyleSheet(R"(
        QLabel#songnamelabel {
        font-family: "Microsoft YaHei", "PingFang SC", "Helvetica Neue", sans-serif;
        font-size: 28px;                /* 大号字体突出歌曲名 */
        font-weight: 600;                /* 半粗体 */
        color: white;                    /* 纯白文字 */
        background-color: rgba(0, 0, 0, 80); /* 半透明黑色背景，增强文字可读性 */
        padding: 10px 20px;               /* 内边距，让背景有呼吸空间 */
        border-radius: 12px;              /* 圆角 */
        letter-spacing: 1px;              /* 字符间距，更显精致 */
        qproperty-alignment: AlignCenter; /* 文字居中（可选） */
    }
)");

    ui->lyricsListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lyricsListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// 自动播放上一首
void MainWindow::PlayPrevSong()
{
    if (m_musicplaylist->isempty()) return;
    if(m_player->isPlaying()) m_autoplay = true;


    if(m_nextmode == Repeat_Play)
    {
        m_player->setPosition(1);
        return;  // 单曲循环就不用重新setSource了,直接把进度条归零就好
    }

    if(m_nextmode == List_Play)
    {
        m_playnum--;
        if(m_playnum == -1) m_playnum = m_musicplaylist->Getsize() - 1;
    }
    else if(m_nextmode == Loop_Play)
    {
        // 移动到随机顺序中的前一个（循环）
        m_shuffleIndex = (m_shuffleIndex - 1 + m_shuffleOrder.size()) % m_shuffleOrder.size();
        m_playnum = m_shuffleOrder[m_shuffleIndex];
    }

    PlaySong();
}

// 自动播放下一首
void MainWindow::PlayNextSong()
{
    if (m_musicplaylist->isempty()) return;
    if(m_player->isPlaying()) m_autoplay = true;

    if(m_nextmode == Repeat_Play)
    {
        m_player->setPosition(1);
        return;  // 单曲循环就不用重新setSource了,直接把进度条归零就好
    }

    if(m_nextmode == List_Play)
    {
        m_playnum = (m_playnum + 1) % m_musicplaylist->Getsize();
    }
    else if(m_nextmode == Loop_Play)
    {
        // 移动到随机顺序中的下一个（循环）
        m_shuffleIndex = (m_shuffleIndex + 1) % m_shuffleOrder.size();
        m_playnum = m_shuffleOrder[m_shuffleIndex];
    }

    PlaySong();
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

// 更新一组新的随机播放数组
void MainWindow::UpdateRandomArray()
{
    int size = m_musicplaylist->Getsize();
    m_shuffleOrder.clear();
    if (size == 0) return;

    // 初始化顺序列表为 0 ~ size-1
    for (int i = 0; i < size; ++i) {
        m_shuffleOrder.append(i);
    }

    // Fisher-Yates 洗牌算法生成随机顺序
    for (int i = size - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        m_shuffleOrder.swapItemsAt(i, j);
    }

    // 找到当前正在播放的歌曲在随机顺序中的位置
    m_shuffleIndex = m_shuffleOrder.indexOf(m_playnum);
}

void MainWindow::PlaySong()
{
    m_player->setSource(m_musicplaylist->Geturl(m_playnum));

    // 加载歌词
    // QString filePath = m_musicplaylist->Geturl(m_playnum).toLocalFile();
    // if (!filePath.isEmpty()) {
    //     loadLyrics(filePath);
    // }

    if(m_autoplay)
    {
        // 等待0.5秒确保音乐已经加载完毕
        QTimer::singleShot(500, [=]() {
            m_autoplay = false;
            m_player->play();
        });
    }
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
    InitLrcParser();

    // 音乐准备完毕信号槽
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::StatusChanged);

    // 音乐播放结束信号槽
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::StateChange);

    // 进度条相关槽函数
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::updateSliderPosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::updateSliderRange);
    connect(ui->Slider, &QSlider::sliderMoved, this, &MainWindow::onProgressSliderMoved);
    // 假设你有一个 QSlider 对象叫做 slider
    connect(ui->Slider, &QSlider::valueChanged, this, [=](int currentValue) {
        // 判断当前值是否等于滑块设定的最大值
        if (currentValue == ui->Slider->maximum()) {
            MusicEnd();
        }
    });

    // 使用音乐播放列表选择播放音乐
    connect(m_musicplaylist, &MusicPlaylist::ChooseMusicpass, this, [this](int id){
        if(m_nextmode == Loop_Play)
        {
            m_playnum = id;
            int idx = m_shuffleOrder.indexOf(m_playnum);
            if (idx == -1) {
                // 如果当前歌曲不在随机顺序列表中（例如播放列表已更新），重新生成随机顺序
                UpdateRandomArray();
            } else {
                m_shuffleIndex = idx;
            }
        }
        else
        {
            m_playnum = id;
        }
        if(m_player->isPlaying()) m_autoplay = true;
        PlaySong();
    });
}

void MainWindow::updateSliderRange(qint64 duration)
{
    ui->Slider->setRange(0, static_cast<int>(duration));
    ui->Slider->setEnabled(duration > 0);
}

void MainWindow::updatalyricsListWidget()
{
    int window_width = this->width();
    ui->lyricsListWidget->setFixedHeight(window_width / 2);
}

void MainWindow::updateSliderPosition(qint64 position)
{
    // 避免在拖动滑块时反复设置位置导致冲突，可使用 blockSignals 或判断是否正在拖动
    if (!ui->Slider->isSliderDown()) {
        ui->Slider->setValue(static_cast<int>(position));
    }
}

void MainWindow::onProgressSliderMoved(int value)
{
    m_player->setPosition(static_cast<qint64>(value));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadLyrics(const QString &musicFilePath)
{
    // 构造歌词文件路径：将音乐文件后缀替换为 .lrc
    QFileInfo info(musicFilePath);
    QString lrcPath = info.absolutePath() + "/" + info.completeBaseName() + ".lrc";

    if (m_lrcParser->parseFile(lrcPath)) {
        m_lyrics = m_lrcParser->lyrics();
        // 清空并填充歌词列表控件
        ui->lyricsListWidget->clear();
        for (const auto &line : m_lyrics) {
            ui->lyricsListWidget->addItem(line.text);
        }
    } else {
        ui->lyricsListWidget->clear();
        QListWidgetItem *noLrcItem = new QListWidgetItem("无歌词");
        QFont font = noLrcItem->font();
        font.setItalic(true);
        noLrcItem->setFont(font);
        noLrcItem->setForeground(QColor(128, 128, 128));
        noLrcItem->setTextAlignment(Qt::AlignCenter);
        ui->lyricsListWidget->addItem(noLrcItem);
    }
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (m_lyrics.isEmpty()) return;

    int index = m_lrcParser->currentIndex(position);
    if (index >= 0 && index < ui->lyricsListWidget->count()) {
        // 高亮当前行
        ui->lyricsListWidget->setCurrentRow(index);
        // 确保当前行可见（滚动到中间）
        ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->currentItem(), QAbstractItemView::PositionAtCenter);
    }
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
    ui->songnamelabel->setText("未知曲目");
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
            ui->songnamelabel->setText(value.toString());
        }
    }

    if(image_flag) ui->imagelabel->setPixmap(QPixmap(":/res/misaka.png"));

    // 解决一些莫名其妙的bug
    m_player->setPosition(1);

    // 设置歌词
    QString filePath = m_musicplaylist->Geturl(m_playnum).toLocalFile();
    if (!filePath.isEmpty()) {
        loadLyrics(filePath);
    }
}

void MainWindow::StateChange(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::StoppedState:
        // MusicEnd();
        break;
    case QMediaPlayer::PlayingState:

        break;
    case QMediaPlayer::PausedState:

        break;
    }
}

void MainWindow::MusicEnd()
{
    // InitButtonIcon(ui->playButton, ":/res/play.png");
    // 自动下一首
    m_autoplay = true;
    if (!m_musicplaylist->isempty()) PlayNextSong();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    UpdateMusicListPosition();
    updatalyricsListWidget();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // if (obj == ui->Slider && event->type() == QEvent::MouseButtonPress) {
    //     QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    //     QSlider *slider = ui->Slider;

    //     // 将鼠标点击的 X 坐标转换为滑块值
    //     int value = QStyle::sliderValueFromPosition(
    //         slider->minimum(),
    //         slider->maximum(),
    //         mouseEvent->pos().x(),
    //         slider->width()
    //         );

    //     // 设置滑块位置
    //     slider->setValue(value);
    //     // 同时设置播放器进度
    //     m_player->setPosition(static_cast<qint64>(value));

    //     return true; // 事件已处理，不再传递
    // }
    // 其他事件交给父类处理
    return QMainWindow::eventFilter(obj, event);
}
