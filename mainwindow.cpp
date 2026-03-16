#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMediaMetaData>
#include <QDir>
#include <QCoreApplication>
#include <QResizeEvent>
#include <QScrollBar>
#include <QMouseEvent>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include "songunit.h"

// 加载样式文件
void MainWindow::loadStyleSheet()
{
    // 获取应用程序目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString stylePath = ":/style.qss";

    QFile styleFile(stylePath);

    if (styleFile.open(QIODevice::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());
        this->setStyleSheet(styleSheet);
        styleFile.close();
    }
}

// 初始化主窗口
void MainWindow::InitWindow()
{
    setWindowTitle("MusicPlayer");
    this->resize(1235, 833);

    // 禁用默认标题栏
    setWindowFlags(Qt::FramelessWindowHint);
    
    // 启用透明背景，支持圆角效果
    // setAttribute(Qt::WA_TranslucentBackground);

    // 禁用自动背景填充
    setAutoFillBackground(false);

    // 加载样式表
    loadStyleSheet();

    ui->imagelabel->setFixedSize(300, 300);
    ui->imagelabel->setScaledContents(true);

    m_sliderPressed = false;
    m_ignoreSliderUpdate = false;
    m_pendingSeek = -1;
    m_isDragging = false;

    // 初始化窗口调整大小相关变量
    m_isResizing = false;
    m_resizeEdge = NoEdge;

    ui->Slider->installEventFilter(this);
    // 为主窗口安装事件过滤器
    this->installEventFilter(this);
}

// 初始化所有按钮
void MainWindow::InitButtons()
{
    InitButtonIcon(ui->prevButton, ":/res/prev song.png");
    InitButtonIcon(ui->playButton, ":/res/play.png");
    InitButtonIcon(ui->nextButton, ":/res/next song.png");
    InitButtonIcon(ui->modeButton, ":/res/list play.png");
    InitButtonIcon(ui->listButton, ":/res/playlist.png");
    InitButtonIcon(ui->minimizeButton, ":/res/Minimize.png");
    InitButtonIcon(ui->maximizeButton, ":/res/Maximize.png");
    InitButtonIcon(ui->closeButton, ":/res/close.png");

    // 实现循环模式按钮功能
    connect(ui->modeButton, &QPushButton::clicked, this, [this](){
        if (!m_playerController) return;
        nextmode mode = m_playerController->GetPlayMode();
        switch(mode)
        {
            case(List_Play):
            {
                m_playerController->SetPlayMode(Repeat_Play);
                InitButtonIcon(ui->modeButton, ":/res/repeat play.png");
            }
            break;
            case(Loop_Play):
            {
                m_playerController->SetPlayMode(List_Play);
                InitButtonIcon(ui->modeButton, ":/res/list play.png");
            }
            break;
            case(Repeat_Play):
            {
                m_playerController->SetPlayMode(Loop_Play);
                InitButtonIcon(ui->modeButton, ":/res/loop play.png");
            }
            break;
        }
    });

    // 实现上一首按钮功能
    connect(ui->prevButton, &QPushButton::clicked, this, [this](){
        if (m_playerController) m_playerController->PlayPrevSong();
    });
    // 实现下一首按钮功能
    connect(ui->nextButton, &QPushButton::clicked, this, [this](){
        if (m_playerController) m_playerController->PlayNextSong();
    });

    // 实现使用playButton按钮控制音乐暂停、播放功能
    connect(ui->playButton, &QPushButton::clicked, this, [this](){
        if(!m_playerController) return;
        QMediaPlayer *player = m_playerController->GetPlayer();
        if(player->isPlaying())
        {
            player->pause();
            InitButtonIcon(ui->playButton, ":/res/play.png");
        }
        else
        {
            player->play();
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

    // 实现最小化按钮功能
    connect(ui->minimizeButton, &QPushButton::clicked, this, [this](){
        this->showMinimized();
    });

    // 实现最大化/窗口化按钮功能
    connect(ui->maximizeButton, &QPushButton::clicked, this, [this](){
        if (this->isMaximized()) {
            this->showNormal();
            InitButtonIcon(ui->maximizeButton, ":/res/Maximize.png");
        } else {
            this->showMaximized();
            InitButtonIcon(ui->maximizeButton, ":/res/Windowed.png");
        }
    });

    // 实现关闭按钮功能
    connect(ui->closeButton, &QPushButton::clicked, this, [this](){
        this->close();
    });
}

// 初始化按钮图标
void MainWindow::InitButtonIcon(QPushButton *button, const QString & path)
{
    button->setFixedSize(30, 30);
    button->setIcon(QIcon(path));
    button->setIconSize(QSize(button->width(), button->height()));
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

    m_musicplaylist = new MusicPlaylist(this);
    m_musicplaylist->hide();
    UpdateMusicListPosition();
    if (m_playerController) {
        m_playerController->InitPlayList(m_musicplaylist);
    }
}

// 初始化播放列表
void MainWindow::InitLrcParser()
{
    // 歌词解析对象
    m_lrcParser = new LrcParser(this);
    if (m_playerController) {
        connect(m_playerController->GetPlayer(), &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    }

    ui->lyricsListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lyricsListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 初始化鼠标滚轮定时器
    m_wheelTimer = new QTimer(this);
    m_wheelTimer->setSingleShot(true);
    m_wheelTimer->setInterval(3000); // 3秒
    connect(m_wheelTimer, &QTimer::timeout, this, &MainWindow::onWheelTimerTimeout);
    
    // 连接歌词点击信号
    connect(ui->lyricsListWidget, &QListWidget::clicked, this, &MainWindow::onLyricsListWidgetClicked);
    
    // 安装事件过滤器以捕获鼠标滚轮事件
    ui->lyricsListWidget->installEventFilter(this);
    // 安装事件过滤器到视口
    ui->lyricsListWidget->viewport()->installEventFilter(this);
    
    m_manualScroll = false;
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
    m_playerController = new PlayerController(this);
    InitButtons();
    InitPlayList();
    InitLrcParser();

    QMediaPlayer *player = m_playerController->GetPlayer();

    // 音乐准备完毕信号槽
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::StatusChanged);

    // 音乐播放结束信号槽
    connect(player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::StateChange);

    // 进度条相关槽函数
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::updateSliderPosition);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::updateSliderRange);

    // 使用音乐播放列表选择播放音乐
    connect(m_musicplaylist, &MusicPlaylist::ChooseMusicpass, m_playerController, &PlayerController::OnChooseMusic);
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
    // 当用户按住进度条拖动，或刚刚手动 seek 完成时，不去更新 UI，避免来回“抢位置”
    if (m_sliderPressed || m_ignoreSliderUpdate) {
        return;
    }

    ui->Slider->setValue(static_cast<int>(position));
}

void MainWindow::onProgressSliderMoved(int value)
{
    Q_UNUSED(value);
    // 拖动时暂时不直接设置播放器位置，统一在鼠标释放时处理
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
        
        // 在歌词末尾添加空白行，确保最后几行歌词也能居中显示
        for (int i = 0; i < 5; ++i) {
            ui->lyricsListWidget->addItem("");
        }
        // 重置手动滚动标志，并立即居中当前行
        m_manualScroll = false;
        m_wheelTimer->stop();  // 停止可能正在运行的定时器

        if (m_playerController) {
            qint64 pos = m_playerController->GetPlayer()->position();
            int idx = m_lrcParser->currentIndex(pos);
            if (idx >= 0 && idx < m_lyrics.size()) {
                ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(idx), QAbstractItemView::PositionAtCenter);
            }
        }

    } else {
         // 无歌词时显示占位项，并重置标志
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
    if (index >= 0 && index < m_lyrics.size()) {
        // 清除所有选中状态
        for (int i = 0; i < ui->lyricsListWidget->count(); ++i) {
            ui->lyricsListWidget->item(i)->setSelected(false);
        }
        // 高亮当前行
        ui->lyricsListWidget->item(index)->setSelected(true);

        // 只有当用户没有手动滚动时，才自动居中
        if (!m_manualScroll) {
            ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(index), QAbstractItemView::PositionAtCenter);
        }
    }
}

void MainWindow::onLyricsListWidgetClicked(QModelIndex index)
{
    // 确保点击的索引在有效范围内
    if (index.row() >= 0 && index.row() < m_lyrics.size()) {
        // 获取点击行对应的时间
        qint64 time = m_lyrics[index.row()].time;
        // 设置播放器位置
        if (m_playerController) {
            QMediaPlayer *player = m_playerController->GetPlayer();
            player->setPosition(time);
            
            // 主动 seek 之后，短时间内忽略播放器发来的 positionChanged，避免旧位置把滑块“拉回去”
            m_ignoreSliderUpdate = true;
            QTimer::singleShot(150, this, [this]() {
                m_ignoreSliderUpdate = false;
            });
        }
        
        // 点击后自动居中显示当前行
        m_manualScroll = false;
        ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(index.row()), QAbstractItemView::PositionAtCenter);
    }
}

void MainWindow::onWheelTimerTimeout()
{
    // 5秒无操作后，恢复自动居中显示当前播放歌词;
    m_manualScroll = false;
    if (!m_lyrics.isEmpty() && m_playerController) {
        qint64 position = m_playerController->GetPlayer()->position();
        int index = m_lrcParser->currentIndex(position);
        if (index >= 0 && index < m_lyrics.size()) {
            ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(index), QAbstractItemView::PositionAtCenter);
        }
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

        // 如果在媒体加载过程中用户已经拖动过一次进度条，这里补一次 seek
        if (m_pendingSeek >= 0 && m_playerController) {
            QMediaPlayer *player = m_playerController->GetPlayer();
            player->setPosition(m_pendingSeek);

            // 应用完毕后清除，并短暂屏蔽 positionChanged，防止“回弹”
            m_ignoreSliderUpdate = true;
            QTimer::singleShot(150, this, [this]() {
                m_ignoreSliderUpdate = false;
            });

            m_pendingSeek = -1;
        }
        break;
    case QMediaPlayer::BufferingMedia:   // 正在缓冲媒体
        break;
    case QMediaPlayer::BufferedMedia:    // 缓冲完成，可播放
        // 理论上 LoadedMedia 分支已经处理 pendingSeek，这里只是兜底
        if (m_pendingSeek >= 0 && m_playerController) {
            QMediaPlayer *player = m_playerController->GetPlayer();
            player->setPosition(m_pendingSeek);

            m_ignoreSliderUpdate = true;
            QTimer::singleShot(150, this, [this]() {
                m_ignoreSliderUpdate = false;
            });

            m_pendingSeek = -1;
        }
        break;
    case QMediaPlayer::StalledMedia:     // 播放停滞（缓冲不足）
        break;
    case QMediaPlayer::EndOfMedia:       // 已到达媒体末尾
        // 如果用户正在按住进度条拖动，则先不切到下一首，
        // 等鼠标释放时再根据最终位置决定是停在当前还是跳到下一曲
        if (!m_sliderPressed) {
            MusicEnd();                  // 正常播放结束时，自动切换到下一首
        }
        break;
    case QMediaPlayer::InvalidMedia:     // 无效媒体
        break;
    }
}

// 更新音乐元数据
void MainWindow::UpdateMetadata()
{
    if (!m_playerController) return;
    QMediaMetaData metaData = m_playerController->GetPlayer()->metaData();

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

    // 设置歌词：根据当前播放索引加载对应 .lrc 文件
    if (!m_musicplaylist) return;

    int currentIndex = m_playerController->GetCurrentIndex();
    if (currentIndex < 0 || currentIndex >= m_musicplaylist->Getsize()) {
        return;
    }

    QString filePath = m_musicplaylist->Geturl(currentIndex).toLocalFile();
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
    if (m_playerController) {
        m_playerController->MusicEnd();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // 绘制带圆角的背景
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    setMask(QRegion(path.toFillPolygon().toPolygon()));

    UpdateMusicListPosition();
    updatalyricsListWidget();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // if(obj == ui->lyricsListWidget) qDebug() << "eventFilter:" << obj << event->type();
    if (obj == ui->Slider) {
        // 鼠标按下：开始一次拖动/点选，不立刻改变播放进度
        if (event->type() == QEvent::MouseButtonPress) {
            m_sliderPressed = true;
            // 让 QSlider 自己处理按下事件（包括点选行为）
            // return QMainWindow::eventFilter(obj, event);
        }

        // 鼠标释放：本次拖动/点选结束，此时统一设置播放器 position
        if (event->type() == QEvent::MouseButtonRelease && m_sliderPressed) {
            QSlider *slider = ui->Slider;

            m_sliderPressed = false;

            int value = slider->value(); // 使用当前滑块位置，不再手动计算

            if (m_playerController) {
                QMediaPlayer *player = m_playerController->GetPlayer();
                QMediaPlayer::MediaStatus status = player->mediaStatus();

                // 如果媒体还在加载中，先记录一个待应用的进度，等 LoadedMedia/BufferedMedia 再真正 seek
                if (status == QMediaPlayer::LoadingMedia ||
                    status == QMediaPlayer::NoMedia)
                {
                    m_pendingSeek = static_cast<qint64>(value);
                }
                else
                {
                    player->setPosition(static_cast<qint64>(value));

                    // 主动 seek 之后，短时间内忽略播放器发来的 positionChanged，避免旧位置把滑块“拉回去”
                    m_ignoreSliderUpdate = true;
                    QTimer::singleShot(150, this, [this]() {
                        m_ignoreSliderUpdate = false;
                    });
                }
            }

            // 如果释放位置在末尾，视为用户主动把歌拖到末尾，此时再切到下一首
            if (value >= slider->maximum()) {
                MusicEnd();
            }

            // 让 QSlider 正常收到 release，避免内部状态（isSliderDown 等）卡住
            // return QMainWindow::eventFilter(obj, event);
        }
    }
    


    // 处理歌词列表的鼠标滚轮事件
    if ((obj == ui->lyricsListWidget || obj == ui->lyricsListWidget->viewport())
        && event->type() == QEvent::Wheel) {
        // qDebug() << "滚轮事件触发，启动定时器";
        m_manualScroll = true;
        m_wheelTimer->start();
        return false;   // 允许事件继续传递给 QListWidget，使其能够滚动
    }

    // 处理窗口拖动和调整大小
    if (obj == this) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // 首先检查是否在调整大小区域
                Edge edge = getEdge(mouseEvent->pos());
                if (edge != NoEdge && mouseEvent->pos().y() >= 50) {
                    // 在非标题栏区域检测到边缘，开始调整大小
                    startResize(edge, mouseEvent->pos());
                    return true;
                }
                
                // 检查鼠标是否在窗口顶部区域（用于拖动）
                if (mouseEvent->pos().y() < 50) {
                    m_isDragging = true;
                    m_dragStartPos = mouseEvent->globalPosition() - this->frameGeometry().topLeft();
                }
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            
            // 如果正在调整大小
            if (m_isResizing) {
                performResize(mouseEvent->pos());
                return true;
            }
            
            // 如果正在拖动窗口
            if (m_isDragging) {
                QPointF newPos = mouseEvent->globalPosition() - m_dragStartPos;
                this->move(newPos.toPoint());
                return true;
            }
            
            // 如果没有按下鼠标按钮，更新光标形状
            if (!(mouseEvent->buttons() & Qt::LeftButton)) {
                Edge edge = getEdge(mouseEvent->pos());
                updateCursor(edge);
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            // 停止拖动和调整大小
            m_isDragging = false;
            if (m_isResizing) {
                m_isResizing = false;
                m_resizeEdge = NoEdge;
                this->unsetCursor();
            }
        }
    }

    // 其他事件交给父类处理
    return QMainWindow::eventFilter(obj, event);
}

// 根据鼠标位置获取边缘类型
MainWindow::Edge MainWindow::getEdge(const QPoint &pos)
{
    Edge edge = NoEdge;
    int x = pos.x();
    int y = pos.y();
    
    // 获取窗口几何形状
    QRect rect = this->rect();
    
    // 检查左右边缘
    if (x <= m_resizeBorderWidth) {
        edge = LeftEdge;
    } else if (x >= rect.width() - m_resizeBorderWidth) {
        edge = RightEdge;
    }
    
    // 检查上下边缘
    if (y <= m_resizeBorderWidth) {
        edge = static_cast<Edge>(edge | TopEdge);
    } else if (y >= rect.height() - m_resizeBorderWidth) {
        edge = static_cast<Edge>(edge | BottomEdge);
    }
    
    return edge;
}

// 根据边缘类型更新鼠标光标
void MainWindow::updateCursor(Edge edge)
{
    Qt::CursorShape cursor = Qt::ArrowCursor;
    
    switch (edge) {
    case LeftEdge:
    case RightEdge:
        cursor = Qt::SizeHorCursor;
        break;
    case TopEdge:
    case BottomEdge:
        cursor = Qt::SizeVerCursor;
        break;
    case TopLeftEdge:
    case BottomRightEdge:
        cursor = Qt::SizeFDiagCursor;
        break;
    case TopRightEdge:
    case BottomLeftEdge:
        cursor = Qt::SizeBDiagCursor;
        break;
    default:
        cursor = Qt::ArrowCursor;
        break;
    }
    
    this->setCursor(cursor);
}

// 开始调整大小
void MainWindow::startResize(Edge edge, const QPoint &pos)
{
    m_isResizing = true;
    m_resizeEdge = edge;
    m_resizeStartPos = pos;
    m_resizeStartGeometry = this->geometry();
}

// 执行调整大小
void MainWindow::performResize(const QPoint &pos)
{
    if (!m_isResizing) return;
    
    QRect geometry = m_resizeStartGeometry;
    QPoint globalPos = this->mapToGlobal(pos);
    QPoint globalStartPos = this->mapToGlobal(m_resizeStartPos);
    int deltaX = globalPos.x() - globalStartPos.x();
    int deltaY = globalPos.y() - globalStartPos.y();
    
    // 根据边缘调整几何形状
    if (m_resizeEdge & LeftEdge) {
        geometry.setLeft(geometry.left() + deltaX);
    }
    if (m_resizeEdge & RightEdge) {
        geometry.setRight(geometry.right() + deltaX);
    }
    if (m_resizeEdge & TopEdge) {
        geometry.setTop(geometry.top() + deltaY);
    }
    if (m_resizeEdge & BottomEdge) {
        geometry.setBottom(geometry.bottom() + deltaY);
    }
    
    // 确保窗口有最小尺寸
    if (geometry.width() < minimumWidth() || geometry.height() < minimumHeight()) {
        return;
    }
    
    this->setGeometry(geometry);
    
    // 更新起始位置为当前位置，用于连续调整
    m_resizeStartPos = pos;
    m_resizeStartGeometry = geometry;
}


