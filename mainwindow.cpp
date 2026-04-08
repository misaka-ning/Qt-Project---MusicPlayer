#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "config/AppConstants.h"

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
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QShortcut>
#include <QStatusBar>
#include <QUrlQuery>
#include <QDialog>
#include <QVBoxLayout>
#include <QtGlobal>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

/** @brief 根据是否有歌曲启用/禁用播放相关控件，列表按钮始终可用；空列表时复位播放按钮图标。 */
void MainWindow::updatePlaybackControlsEnabled(bool enabled)
{
    if (!ui) return;

    if (ui->prevButton) ui->prevButton->setEnabled(enabled);
    if (ui->playButton) ui->playButton->setEnabled(enabled);
    if (ui->nextButton) ui->nextButton->setEnabled(enabled);
    if (ui->modeButton) ui->modeButton->setEnabled(enabled);
    if (ui->Slider) ui->Slider->setEnabled(enabled);

    // 列表按钮始终可用（用于“空列表时也能打开列表窗口看占位”）
    if (ui->listButton) ui->listButton->setEnabled(true);

    // 空列表时把播放按钮状态复位到“播放”
    if (!enabled && ui->playButton) {
        InitButtonIcon(ui->playButton, ":/res/play.png");
    }
}

/** @brief 显示或隐藏“当前没有加载的音乐”占位标签，显示时置顶。 */
void MainWindow::updateEmptyOverlayVisible(bool visible)
{
    if (!m_emptyOverlayLabel) return;
    m_emptyOverlayLabel->setVisible(visible);
    if (visible) {
        m_emptyOverlayLabel->raise();
    }
}

/** @brief 从资源 :/style.qss 读取样式表并应用到主窗口。 */
void MainWindow::loadStyleSheet()
{
    QFile styleFile(":/style.qss");

    if (styleFile.open(QIODevice::ReadOnly)) {
        QString styleSheet = QString::fromUtf8(styleFile.readAll());
        this->setStyleSheet(styleSheet);
        styleFile.close();
    }
}

/** @brief 初始化主窗口：标题、大小、无边框、样式、封面尺寸、空列表 overlay、进度条与窗口状态、事件过滤器。 */
void MainWindow::InitWindow()
{
    setWindowTitle("MusicPlayer");
    this->resize(AppConstants::Ui::MainWindowWidth, AppConstants::Ui::MainWindowHeight);

    setWindowFlags(Qt::FramelessWindowHint);
    setAutoFillBackground(false);

    // 加载样式表
    loadStyleSheet();

    ui->imagelabel->setFixedSize(AppConstants::Ui::CoverSize, AppConstants::Ui::CoverSize);
    ui->imagelabel->setScaledContents(true);

    // 空列表占位 overlay（不拦截鼠标事件，避免挡住按钮）
    m_emptyOverlayLabel = new QLabel(this);
    m_emptyOverlayLabel->setText(QStringLiteral("当前没有加载的音乐"));
    m_emptyOverlayLabel->setAlignment(Qt::AlignCenter);
    m_emptyOverlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_emptyOverlayLabel->setStyleSheet(
        "QLabel{"
        "color: rgba(255,255,255,200);"
        "background-color: rgba(0,0,0,120);"
        "border-radius: 12px;"
        "padding: 14px 22px;"
        "font-size: 18px;"
        "}"
    );
    m_emptyOverlayLabel->hide();

    m_sliderPressed = false;
    m_ignoreSliderUpdate = false;
    m_pendingSeek = -1;
    m_isDragging = false;
    m_moremenuwindow = new MoreMenu(this);
    m_moremenuwindow->hide();
    m_moremenuwindow->setLoginState(false, QString());
    connect(m_moremenuwindow, &MoreMenu::addMusicClicked, this, &MainWindow::onAddMusicFromMoreMenu);
    connect(m_moremenuwindow, &MoreMenu::removeCurrentSongClicked, this, &MainWindow::onRemoveCurrentSongFromMoreMenu);
    connect(m_moremenuwindow, &MoreMenu::authNeteaseClicked, this, &MainWindow::onNeteaseAuthButtonFromMoreMenu);

    m_coverNam = new QNetworkAccessManager(this);

    // 初始化窗口调整大小相关变量
    m_isResizing = false;
    m_resizeEdge = NoEdge;

    ui->Slider->installEventFilter(this);
    ui->Slider->setCursor(Qt::PointingHandCursor);
    this->installEventFilter(this);
    if (qApp) {
        qApp->installEventFilter(this);
    }
}

void MainWindow::onAddMusicFromMoreMenu()
{
    if (!m_playerController) return;

    const QString filter = QStringLiteral("Audio Files (*.mp3 *.wav *.flac *.aac *.ogg *.m4a *.wma);;All Files (*.*)");
    const QStringList files = QFileDialog::getOpenFileNames(this, QStringLiteral("选择音乐文件"), QString(), filter);
    if (files.isEmpty()) return;

    m_playerController->AddLocalFiles(files);
}

void MainWindow::onRemoveCurrentSongFromMoreMenu()
{
    if (!m_playerController) return;
    const bool ok = m_playerController->removeCurrentSong();
    if (statusBar()) {
        statusBar()->showMessage(ok ? QStringLiteral("已删除当前歌曲") : QStringLiteral("删除失败：当前无可删除歌曲"), 2000);
    }
    if (m_moremenuwindow) m_moremenuwindow->hide();
}

void MainWindow::onNeteaseAuthButtonFromMoreMenu(bool loggedIn)
{
    if (!m_cloudService) return;
    if (loggedIn) {
        m_neteaseQrLoginActive = false;
        m_currentNeteaseQrUnikey.clear();
        closeNeteaseQrDialog();
        m_cloudService->logoutNetease(++m_cloudRequestSeq);
        if (statusBar()) statusBar()->showMessage(QStringLiteral("正在退出网易云登录..."), 2000);
    } else {
        startNeteaseQrLogin();
    }
    if (m_moremenuwindow) m_moremenuwindow->hide();
}

void MainWindow::startNeteaseQrLogin()
{
    if (!m_cloudService) return;
    m_neteaseQrLoginActive = true;
    m_currentNeteaseQrUnikey.clear();
    if (m_moremenuwindow) m_moremenuwindow->setLoginState(false, QString());
    if (statusBar()) statusBar()->showMessage(QStringLiteral("正在准备网易云登录二维码..."), 2500);
    m_cloudService->requestNeteaseQrUnikey(++m_cloudRequestSeq);
}

void MainWindow::scheduleNeteaseQrPoll(int delayMs)
{
    if (!m_neteaseQrLoginActive || m_currentNeteaseQrUnikey.isEmpty() || !m_cloudService) return;
    QTimer::singleShot(qMax(300, delayMs), this, [this]() {
        if (!m_neteaseQrLoginActive || m_currentNeteaseQrUnikey.isEmpty() || !m_cloudService) return;
        m_cloudService->checkNeteaseQrStatus(m_currentNeteaseQrUnikey, ++m_cloudRequestSeq);
    });
}

void MainWindow::showNeteaseLoginErrorWithRetry(const QString &message)
{
    closeNeteaseQrDialog();
    const QString readable = message.trimmed().isEmpty()
        ? QStringLiteral("网易云登录失败，请稍后重试。")
        : message.trimmed();
    const auto ret = QMessageBox::warning(this,
                                          QStringLiteral("网易云登录失败"),
                                          readable,
                                          QMessageBox::Retry | QMessageBox::Cancel,
                                          QMessageBox::Retry);
    if (ret == QMessageBox::Retry) {
        startNeteaseQrLogin();
        return;
    }
    m_neteaseQrLoginActive = false;
    m_currentNeteaseQrUnikey.clear();
}

void MainWindow::showNeteaseQrDialog(const QUrl &qrDataUrl)
{
    if (!m_neteaseQrDialog) {
        m_neteaseQrDialog = new QDialog(this);
        m_neteaseQrDialog->setWindowTitle(QStringLiteral("网易云扫码登录"));
        m_neteaseQrDialog->setModal(false);
        m_neteaseQrDialog->setAttribute(Qt::WA_DeleteOnClose, false);

        auto *layout = new QVBoxLayout(m_neteaseQrDialog);
        auto *tipLabel = new QLabel(QStringLiteral("请使用手机网易云音乐 App 扫码并确认登录"), m_neteaseQrDialog);
        tipLabel->setAlignment(Qt::AlignCenter);
        m_neteaseQrImageLabel = new QLabel(QStringLiteral("正在加载二维码..."), m_neteaseQrDialog);
        m_neteaseQrImageLabel->setAlignment(Qt::AlignCenter);
        m_neteaseQrImageLabel->setFixedSize(AppConstants::Ui::NeteaseQrImageSize, AppConstants::Ui::NeteaseQrImageSize);
        m_neteaseQrImageLabel->setStyleSheet(QStringLiteral("QLabel{background:#111;color:#ddd;border:1px solid #333;border-radius:8px;}"));
        layout->addWidget(tipLabel);
        layout->addWidget(m_neteaseQrImageLabel, 0, Qt::AlignCenter);
        m_neteaseQrDialog->setLayout(layout);
        m_neteaseQrDialog->setFixedSize(AppConstants::Ui::NeteaseQrDialogWidth, AppConstants::Ui::NeteaseQrDialogHeight);
    }

    if (m_neteaseQrImageLabel) {
        m_neteaseQrImageLabel->setText(QStringLiteral("正在加载二维码..."));
        m_neteaseQrImageLabel->setPixmap(QPixmap());
    }
    m_neteaseQrDialog->show();
    m_neteaseQrDialog->raise();
    m_neteaseQrDialog->activateWindow();

    if (!m_coverNam || !qrDataUrl.isValid() || qrDataUrl.isEmpty()) return;
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("size"), QStringLiteral("320x320"));
    query.addQueryItem(QStringLiteral("data"), qrDataUrl.toString(QUrl::FullyEncoded));
    QUrl imageUrl(QStringLiteral("https://api.qrserver.com/v1/create-qr-code/"));
    imageUrl.setQuery(query);

    QNetworkReply *reply = m_coverNam->get(QNetworkRequest(imageUrl));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (!m_neteaseQrImageLabel) return;
        if (reply->error() != QNetworkReply::NoError) {
            m_neteaseQrImageLabel->setText(QStringLiteral("二维码加载失败，请重试"));
            return;
        }
        QPixmap px;
        if (!px.loadFromData(reply->readAll())) {
            m_neteaseQrImageLabel->setText(QStringLiteral("二维码解析失败，请重试"));
            return;
        }
        m_neteaseQrImageLabel->setPixmap(px.scaled(320, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
}

void MainWindow::closeNeteaseQrDialog()
{
    if (m_neteaseQrDialog && m_neteaseQrDialog->isVisible()) {
        m_neteaseQrDialog->close();
    }
}

void MainWindow::onNeteaseQrUnikeyReady(quint64 requestId, bool ok, const QString& message, const QString& unikey)
{
    Q_UNUSED(requestId);
    if (!ok || unikey.trimmed().isEmpty()) {
        showNeteaseLoginErrorWithRetry(message.isEmpty() ? QStringLiteral("获取二维码失败。") : message);
        return;
    }
    m_currentNeteaseQrUnikey = unikey.trimmed();
    m_cloudService->requestNeteaseQrUrl(m_currentNeteaseQrUnikey, ++m_cloudRequestSeq);
}

void MainWindow::onNeteaseQrUrlReady(quint64 requestId, bool ok, const QString& message, const QString& unikey, const QUrl& qrUrl)
{
    Q_UNUSED(requestId);
    if (!ok || !qrUrl.isValid() || qrUrl.isEmpty()) {
        showNeteaseLoginErrorWithRetry(message.isEmpty() ? QStringLiteral("获取二维码链接失败。") : message);
        return;
    }
    m_currentNeteaseQrUnikey = unikey.trimmed();
    if (statusBar()) {
        statusBar()->showMessage(QStringLiteral("请使用网易云音乐 App 扫码登录"), 3500);
    }
    showNeteaseQrDialog(qrUrl);
    if (m_neteaseQrLoginActive) {
        scheduleNeteaseQrPoll(1200);
    }
}

void MainWindow::onNeteaseQrStatus(quint64 requestId,
                                   bool ok,
                                   const QString& message,
                                   const QString& unikey,
                                   const QString& status,
                                   int statusCode,
                                   bool loggedIn,
                                   const QString& nickname)
{
    Q_UNUSED(requestId);
    Q_UNUSED(statusCode);
    if (!m_neteaseQrLoginActive) return;
    if (!m_currentNeteaseQrUnikey.isEmpty() && unikey != m_currentNeteaseQrUnikey) return;

    if (!ok) {
        showNeteaseLoginErrorWithRetry(message.isEmpty() ? QStringLiteral("二维码状态查询失败。") : message);
        return;
    }

    const QString state = status.trimmed().toLower();
    if (state == QStringLiteral("authorized") || loggedIn) {
        m_neteaseQrLoginActive = false;
        m_currentNeteaseQrUnikey.clear();
        closeNeteaseQrDialog();
        const QString shownName = nickname.trimmed().isEmpty() ? QStringLiteral("网易云用户") : nickname.trimmed();
        if (m_moremenuwindow) m_moremenuwindow->setLoginState(true, shownName);
        if (statusBar()) statusBar()->showMessage(QStringLiteral("网易云已登录：%1").arg(shownName), 5000);
        QMessageBox::information(this, QStringLiteral("登录成功"), QStringLiteral("网易云登录成功：%1").arg(shownName));
        return;
    }
    if (state == QStringLiteral("expired")) {
        showNeteaseLoginErrorWithRetry(QStringLiteral("二维码已过期，请重试。"));
        return;
    }

    if (statusBar()) {
        statusBar()->showMessage(message.trimmed().isEmpty() ? QStringLiteral("等待扫码...") : message.trimmed(), 1500);
    }
    scheduleNeteaseQrPoll(1800);
}

void MainWindow::onNeteaseAuthStatusReady(quint64 requestId, bool ok, const QString& message, bool loggedIn, const QString& nickname)
{
    Q_UNUSED(requestId);
    if (!m_moremenuwindow) return;

    if (!ok) {
        m_moremenuwindow->setLoginState(false, QString());
        return;
    }

    if (loggedIn) {
        const QString shownName = nickname.trimmed().isEmpty() ? QStringLiteral("网易云用户") : nickname.trimmed();
        m_moremenuwindow->setLoginState(true, shownName);
        if (statusBar()) {
            statusBar()->showMessage(QStringLiteral("网易云已自动登录：%1").arg(shownName), 2500);
        }
    } else {
        Q_UNUSED(message);
        m_moremenuwindow->setLoginState(false, QString());
    }
}

void MainWindow::onNeteaseLogoutFinished(quint64 requestId, bool ok, const QString& message)
{
    Q_UNUSED(requestId);
    if (ok) {
        if (m_moremenuwindow) m_moremenuwindow->setLoginState(false, QString());
        if (statusBar()) statusBar()->showMessage(QStringLiteral("已退出网易云登录"), 2500);
        QMessageBox::information(this, QStringLiteral("退出成功"), QStringLiteral("已退出网易云登录"));
        return;
    }
    const QString tip = message.trimmed().isEmpty() ? QStringLiteral("退出登录失败") : message.trimmed();
    if (statusBar()) statusBar()->showMessage(QStringLiteral("退出失败：%1").arg(tip), 4000);
    QMessageBox::warning(this, QStringLiteral("退出失败"), tip);
}

/** @brief 设置各按钮图标并连接信号：模式切换、上一首/下一首、播放/暂停、列表、最小化/最大化/关闭。 */
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
    InitButtonIcon(ui->moreButton, ":/res/more.png");

    connect(ui->modeButton, &QPushButton::clicked, this, [this](){
        if (!m_playerController) return;
        nextmode mode = m_playerController->GetPlayMode();
        if (mode == List_Play) { m_playerController->SetPlayMode(Repeat_Play); InitButtonIcon(ui->modeButton, ":/res/repeat play.png"); }
        else if (mode == Loop_Play) { m_playerController->SetPlayMode(List_Play); InitButtonIcon(ui->modeButton, ":/res/list play.png"); }
        else { m_playerController->SetPlayMode(Loop_Play); InitButtonIcon(ui->modeButton, ":/res/loop play.png"); }
    });
    connect(ui->prevButton, &QPushButton::clicked, this, [this](){ if (m_playerController) m_playerController->PlayPrevSong(); });
    connect(ui->nextButton, &QPushButton::clicked, this, [this](){ if (m_playerController) m_playerController->PlayNextSong(); });
    connect(ui->playButton, &QPushButton::clicked, this, [this](){
        if (!m_playerController) return;
        QMediaPlayer *player = m_playerController->GetPlayer();
        if (player->isPlaying()) { player->pause(); InitButtonIcon(ui->playButton, ":/res/play.png"); }
        else { player->play(); InitButtonIcon(ui->playButton, ":/res/stop.png"); }
    });
    connect(ui->listButton, &QPushButton::clicked, this, [this](){ togglePlaylist(); });
    connect(ui->minimizeButton, &QPushButton::clicked, this, [this](){ showMinimized(); });
    connect(ui->maximizeButton, &QPushButton::clicked, this, [this](){
        if (isMaximized()) { showNormal(); InitButtonIcon(ui->maximizeButton, ":/res/Maximize.png"); }
        else { showMaximized(); InitButtonIcon(ui->maximizeButton, ":/res/Windowed.png"); }
    });
    connect(ui->closeButton, &QPushButton::clicked, this, [this](){ close(); });
    connect(ui->moreButton, &QPushButton::clicked, this, &MainWindow::moremenubuttonclick);
}

/** @brief 设置按钮固定 30x30 及图标与图标尺寸。 */
void MainWindow::InitButtonIcon(QPushButton *button, const QString & path)
{
    button->setFixedSize(AppConstants::Ui::MoreMenuButtonSize, AppConstants::Ui::MoreMenuButtonSize);
    button->setIcon(QIcon(path));
    button->setIconSize(QSize(button->width(), button->height()));
}

/** @brief 创建 MusicList 目录（若不存在）、MusicPlaylist 控件，更新位置并交给 PlayerController 初始化。 */
void MainWindow::InitPlayList()
{
    QString musicListPath = QCoreApplication::applicationDirPath() + "/MusicList";
    QDir dir;
    if (!dir.exists(musicListPath))
        dir.mkdir(musicListPath);

    m_musicplaylist = new MusicPlaylist(this);
    m_musicplaylist->hide();
    UpdateMusicListPosition();
    if (m_playerController) {
        m_playerController->InitPlayList(m_musicplaylist);
    }
}

/** @brief 创建 LrcParser、连接 positionChanged、设置歌词列表滚动条与滚轮定时器、点击槽、事件过滤器。 */
void MainWindow::InitLrcParser()
{
    m_lrcParser = new LrcParser(this);
    if (m_playerController)
        connect(m_playerController->GetPlayer(), &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);

    ui->lyricsListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->lyricsListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_wheelTimer = new QTimer(this);
    m_wheelTimer->setSingleShot(true);
    m_wheelTimer->setInterval(3000);
    connect(m_wheelTimer, &QTimer::timeout, this, &MainWindow::onWheelTimerTimeout);
    connect(ui->lyricsListWidget, &QListWidget::clicked, this, &MainWindow::onLyricsListWidgetClicked);
    ui->lyricsListWidget->installEventFilter(this);
    ui->lyricsListWidget->viewport()->installEventFilter(this);
    m_manualScroll = false;
}

/** @brief 根据当前窗口宽高计算播放列表目标位置与高度并设置。 */
void MainWindow::UpdateMusicListPosition()
{
    int window_width = this->width();
    int window_height = this->height();
    int target_x = window_width - 390;
    int target_h = window_height - 300;
    const QPoint targetPos(target_x, 100);
    if (m_musicplaylist) {
        m_musicplaylist->setFixedHeight(target_h);
        m_musicplaylist->setTargetPos(targetPos);
    }
}

/** @brief 若播放列表可见则带动画隐藏，否则先更新位置再带动画显示。 */
void MainWindow::togglePlaylist()
{
    if (!m_musicplaylist) return;

    if (m_musicplaylist->isVisible()) {
        m_musicplaylist->hideAnimated();
    } else {
        UpdateMusicListPosition();
        m_musicplaylist->showAnimated();
    }
}

/** @brief 若播放列表当前可见则带动画隐藏。 */
void MainWindow::hidePlaylistIfVisible()
{
    if (!m_musicplaylist) return;
    if (!m_musicplaylist->isVisible()) return;
    m_musicplaylist->hideAnimated();
}

/** @brief 构造：setupUi、InitWindow、创建 PlayerController、连接可用性信号、InitButtons/PlayList/LrcParser、连接播放器与列表信号、初始化 overlay 与控件状态。 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    InitWindow();
    m_playerController = new PlayerController(this);
    m_cloudService = new CloudMusicService(this);
    m_cloudRequestSeq = 0;
    m_cloudSearchWindow = new CloudSearchWindow(this);

    // 播放列表可用性变化：统一更新 overlay 与控件可用性
    connect(m_playerController, &PlayerController::playlistAvailabilityChanged, this, [this](bool hasSongs) {
        updateEmptyOverlayVisible(!hasSongs);
        updatePlaybackControlsEnabled(hasSongs);
    });

    InitButtons();
    InitPlayList();
    InitLrcParser();

    connect(ui->cloudSearchButton, &QPushButton::clicked, this, &MainWindow::onCloudSearchRequested);
    connect(ui->cloudSearchEdit, &QLineEdit::returnPressed, this, &MainWindow::onCloudSearchRequested);
    auto *focusSearchShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+F")), this);
    connect(focusSearchShortcut, &QShortcut::activated, this, [this]() {
        if (ui && ui->cloudSearchEdit) {
            ui->cloudSearchEdit->setFocus();
            ui->cloudSearchEdit->selectAll();
        }
    });
    connect(m_cloudService, &CloudMusicService::searchFinished, this, &MainWindow::onCloudSearchFinished);
    connect(m_cloudService, &CloudMusicService::playUrlReady, this, &MainWindow::onCloudPlayUrlReady);
    connect(m_cloudService, &CloudMusicService::lyricsReady, this, &MainWindow::onCloudLyricsReady);
    connect(m_cloudService, &CloudMusicService::neteaseQrUnikeyReady, this, &MainWindow::onNeteaseQrUnikeyReady);
    connect(m_cloudService, &CloudMusicService::neteaseQrUrlReady, this, &MainWindow::onNeteaseQrUrlReady);
    connect(m_cloudService, &CloudMusicService::neteaseQrStatus, this, &MainWindow::onNeteaseQrStatus);
    connect(m_cloudService, &CloudMusicService::neteaseAuthStatusReady, this, &MainWindow::onNeteaseAuthStatusReady);
    connect(m_cloudService, &CloudMusicService::neteaseLogoutFinished, this, &MainWindow::onNeteaseLogoutFinished);
    connect(m_cloudService, &CloudMusicService::requestFailed, this, &MainWindow::onCloudRequestFailed);
    connect(m_playerController, &PlayerController::cloudTrackResolveRequested,
            this, &MainWindow::onCloudTrackResolveRequested);
    connect(m_cloudSearchWindow, &CloudSearchWindow::songActivated,
            this, &MainWindow::onCloudTrackResolveRequested);

    // 启动后主动恢复网易云登录态（若后端会话仍有效）
    m_cloudService->fetchNeteaseAuthStatus(++m_cloudRequestSeq);

    QMediaPlayer *player = m_playerController->GetPlayer();

    // 音乐准备完毕信号槽
    connect(player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::StatusChanged);

    // 音乐播放结束信号槽
    connect(player, &QMediaPlayer::playbackStateChanged, this, &MainWindow::StateChange);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(player, &QMediaPlayer::errorOccurred, this, [this, player](QMediaPlayer::Error error, const QString& errorString) {
        Q_UNUSED(error);
        const int currentIndex = m_playerController ? m_playerController->GetCurrentIndex() : -1;
        const QString sid = (m_musicplaylist && currentIndex >= 0 && currentIndex < m_musicplaylist->Getsize())
            ? m_musicplaylist->cloudSongIdAt(currentIndex)
            : QString();
        const QString src = player ? player->source().toString() : QString();
        const QString err = errorString.trimmed();
        const QString errLower = err.toLower();
        const bool isHttp403Or404 = errLower.contains(QStringLiteral(" 403"))
            || errLower.contains(QStringLiteral("403 "))
            || errLower.contains(QStringLiteral(" 404"))
            || errLower.contains(QStringLiteral("404 "))
            || errLower.contains(QStringLiteral("forbidden"))
            || errLower.contains(QStringLiteral("not found"));
        if (isHttp403Or404 && !sid.isEmpty() && (sid != m_lastCloudRetrySongId || src != m_lastCloudRetrySourceUrl)) {
            m_lastCloudRetrySongId = sid;
            m_lastCloudRetrySourceUrl = src;
            statusBar()->showMessage(QStringLiteral("播放失败，正在刷新云端链接..."), 2500);
            onCloudTrackResolveRequested(sid);
            return;
        }
        statusBar()->showMessage(QStringLiteral("播放失败：%1").arg(err.isEmpty()
                                                       ? QStringLiteral("未知错误")
                                                       : err),
                                 5000);
    });
#else
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this, player](QMediaPlayer::Error error) {
        Q_UNUSED(error);
        const QString err = player ? player->errorString() : QString();
        statusBar()->showMessage(QStringLiteral("播放失败：%1").arg(err.trimmed().isEmpty()
                                                       ? QStringLiteral("未知错误")
                                                       : err),
                                 5000);
    });
#endif

    // 进度条相关槽函数
    connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::updateSliderPosition);
    connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::updateSliderRange);

    // 使用音乐播放列表选择播放音乐
    connect(m_musicplaylist, &MusicPlaylist::ChooseMusicpass, m_playerController, &PlayerController::OnChooseMusic);

    // 初始化一次空/非空状态（避免错过 InitPlayList 内部 emit）
    const bool hasSongs = (m_musicplaylist && !m_musicplaylist->isempty());
    updateEmptyOverlayVisible(!hasSongs);
    updatePlaybackControlsEnabled(hasSongs);
}

void MainWindow::onCloudSearchRequested()
{
    if (!m_cloudService || !ui || !ui->cloudSearchEdit) return;
    const QString keyword = ui->cloudSearchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("请输入要搜索的关键词"), 2500);
        return;
    }

    const quint64 requestId = ++m_cloudRequestSeq;
    statusBar()->showMessage(QStringLiteral("正在搜索：%1").arg(keyword), 1500);
    m_cloudService->searchSongs(keyword, 1, 20, requestId);
}

void MainWindow::onCloudSearchFinished(quint64 requestId, const QVector<CloudMusicService::CloudSongBrief>& songs)
{
    Q_UNUSED(requestId);
    if (!m_cloudSearchWindow) return;
    m_cloudSearchWindow->setResults(songs);
    m_cloudSearchWindow->show();
    m_cloudSearchWindow->raise();
    m_cloudSearchWindow->activateWindow();
    if (songs.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("未找到结果"), 2500);
    } else {
        statusBar()->showMessage(QStringLiteral("搜索完成：%1 条").arg(songs.size()), 3000);
    }
}

QString MainWindow::cloudLyricsDirPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("CloudLyrics"));
}

QString MainWindow::cloudLyricsLrcPath(const QString &songId) const
{
    return QDir(cloudLyricsDirPath()).filePath(songId + QStringLiteral(".lrc"));
}

QString MainWindow::cloudLyricsStubMusicPath(const QString &songId) const
{
    return QDir(cloudLyricsDirPath()).filePath(songId + QStringLiteral(".mp3"));
}

bool MainWindow::saveCloudLyricsToLocal(const QString &songId, const QString &lrcText)
{
    if (songId.trimmed().isEmpty()) return false;
    QDir dir(cloudLyricsDirPath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) return false;

    QFile file(cloudLyricsLrcPath(songId));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    file.write(lrcText.toUtf8());
    file.close();
    return true;
}

bool MainWindow::loadCloudLyricsFromLocal(const QString &songId)
{
    if (songId.trimmed().isEmpty()) return false;
    if (!QFile::exists(cloudLyricsLrcPath(songId))) return false;
    loadLyrics(cloudLyricsStubMusicPath(songId));
    return true;
}

void MainWindow::fetchCloudCoverArt(const QUrl& coverUrl, int playlistIndex,
                                   const QString& title, const QString& artist)
{
    if (!m_coverNam || !coverUrl.isValid() || coverUrl.isEmpty() || playlistIndex < 0) return;
    QUrl requestUrl = coverUrl;
    if (m_cloudService) {
        QUrl proxyBase(QStringLiteral("http://127.0.0.1:8000/cover/fetch"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("url"), coverUrl.toString());
        proxyBase.setQuery(query);
        requestUrl = proxyBase;
    }

    QNetworkRequest req(requestUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36"));
    req.setRawHeader("Referer", "https://music.163.com/");

    QNetworkReply *reply = m_coverNam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, playlistIndex, title, artist]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QPixmap cover;
        cover.loadFromData(reply->readAll());
        if (cover.isNull()) return;
        if (m_playerController) m_playerController->applyCoverToPlaylistIndex(playlistIndex, cover, title, artist);
        if (m_playerController && playlistIndex == m_playerController->GetCurrentIndex()) {
            ui->imagelabel->setPixmap(cover);
        }
    });
}

void MainWindow::onCloudTrackResolveRequested(const QString& songId)
{
    if (!m_cloudService || songId.trimmed().isEmpty()) return;
    const quint64 requestId = ++m_cloudRequestSeq;
    m_playUrlRequestToSongId.insert(requestId, songId.trimmed());
    statusBar()->showMessage(QStringLiteral("正在获取播放地址..."), 1500);
    m_cloudService->fetchPlayableUrl(songId.trimmed(), requestId);
}

void MainWindow::onCloudPlayUrlReady(quint64 requestId,
                                     const QString& songId,
                                     const QUrl& playUrl,
                                     const QString& title,
                                     const QString& artist,
                                     const QUrl& coverUrl)
{
    if (!m_playerController) return;

    m_playUrlRequestToSongId.remove(requestId);
    if (!songId.trimmed().isEmpty() && songId.trimmed() == m_lastCloudRetrySongId) {
        // 已成功拿到新直链，允许后续在该歌曲再次出现错误时重试一次。
        m_lastCloudRetrySourceUrl.clear();
    }
    const QPixmap cover(":/res/misaka.png");
    const int idx = m_playerController->addOrUpdateCloudTrackAndPlay(songId, playUrl, title, artist, cover);
    if (idx >= 0) {
        statusBar()->showMessage(QStringLiteral("正在播放：%1 - %2").arg(artist, title), 3000);
    }

    if (idx >= 0 && coverUrl.isValid() && !coverUrl.isEmpty()) {
        fetchCloudCoverArt(coverUrl, idx, title, artist);
    }

    if (!songId.trimmed().isEmpty()) {
        if (!loadCloudLyricsFromLocal(songId)) {
            const quint64 lyrReq = ++m_cloudRequestSeq;
            m_latestLyricsRequestId = lyrReq;
            m_pendingLyricsSongId = songId;
            m_cloudService->fetchLyrics(songId, lyrReq);
        }
    }
}

void MainWindow::onCloudRequestFailed(quint64 requestId, CloudMusicService::RequestError error, const QString& message)
{
    Q_UNUSED(error);
    m_playUrlRequestToSongId.remove(requestId);
    statusBar()->showMessage(QStringLiteral("云端请求失败：%1").arg(message), 4000);
}

void MainWindow::onCloudLyricsReady(quint64 requestId, const QString& songId, const QString& lyrics)
{
    if (requestId != m_latestLyricsRequestId) return;
    if (!m_pendingLyricsSongId.isEmpty() && !songId.isEmpty() && songId != m_pendingLyricsSongId) return;
    if (songId.trimmed().isEmpty()) return;

    saveCloudLyricsToLocal(songId, lyrics);
    loadLyrics(cloudLyricsStubMusicPath(songId));
}

/** @brief 槽：播放时长变化时设置进度条范围，根据 duration 启用/禁用。 */
void MainWindow::updateSliderRange(qint64 duration)
{
    ui->Slider->setRange(0, static_cast<int>(duration));
    ui->Slider->setEnabled(duration > 0);
}

/** @brief 根据窗口宽度设置歌词列表控件高度（宽的一半）。 */
void MainWindow::updatalyricsListWidget()
{
    int window_width = this->width();
    ui->lyricsListWidget->setFixedHeight(window_width / 2);
}

/** @brief 槽：播放位置变化时同步到进度条；拖动或 ignore 窗口内不更新。 */
void MainWindow::updateSliderPosition(qint64 position)
{
    if (m_sliderPressed || m_ignoreSliderUpdate) return;
    ui->Slider->setValue(static_cast<int>(position));
}

/** 占位槽：进度条 seek 在 eventFilter 的 MouseButtonRelease 中统一处理。 */
void MainWindow::onProgressSliderMoved(int value)
{
    Q_UNUSED(value);
}

/** @brief 析构：释放 UI。 */
MainWindow::~MainWindow()
{
    delete ui;
}

/** @brief 根据音乐路径查找同目录同名 .lrc，解析后填充歌词列表或显示“无歌词”；重置手动滚动并停止滚轮定时器。 */
void MainWindow::loadLyrics(const QString &musicFilePath)
{
    QFileInfo info(musicFilePath);
    QString lrcPath = info.absolutePath() + "/" + info.completeBaseName() + ".lrc";

    bool ok = m_lrcParser->parseFile(lrcPath);
    m_lyrics = m_lrcParser->lyrics();   // 同步歌词数据

    ui->lyricsListWidget->clear();
    if (ok && !m_lyrics.isEmpty()) {
        // 有歌词：填充真实歌词行
        for (const auto &line : m_lyrics) {
            ui->lyricsListWidget->addItem(line.text);
        }
        // 添加空白行，便于最后几行居中
        for (int i = 0; i < 5; ++i) {
            ui->lyricsListWidget->addItem("");
        }
        // 重置手动滚动标志，并居中当前行
        m_manualScroll = false;
        if (m_playerController) {
            qint64 pos = m_playerController->GetPlayer()->position();
            int idx = m_lrcParser->currentIndex(pos);
            if (idx >= 0 && idx < m_lyrics.size()) {
                ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(idx), QAbstractItemView::PositionAtCenter);
            }
        }
    } else {
        // 无歌词：显示占位项
        QListWidgetItem *noLrcItem = new QListWidgetItem("无歌词");
        QFont font = noLrcItem->font();
        font.setItalic(true);
        noLrcItem->setFont(font);
        noLrcItem->setForeground(QColor(128, 128, 128));
        noLrcItem->setTextAlignment(Qt::AlignCenter);
        ui->lyricsListWidget->addItem(noLrcItem);
        m_manualScroll = false;   // 同样重置手动滚动标志
    }

    // 停止可能正在运行的滚轮定时器
    m_wheelTimer->stop();
}

/** @brief 槽：播放位置变化时高亮对应歌词行，非手动滚动时自动居中。 */
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

/** @brief 槽：点击某行歌词时跳转到对应时间并短暂忽略 positionChanged，居中该行。 */
void MainWindow::onLyricsListWidgetClicked(QModelIndex index)
{
    if (index.row() >= 0 && index.row() < m_lyrics.size()) {
        // 获取点击行对应的时间
        qint64 time = m_lyrics[index.row()].time;
        // 设置播放器位置
        if (m_playerController) {
            QMediaPlayer *player = m_playerController->GetPlayer();
            player->setPosition(time);
            
            // 主动 seek 之后，短时间内忽略播放器发来的 positionChanged，避免旧位置把滑块“拉回去”
            m_ignoreSliderUpdate = true;
            QTimer::singleShot(AppConstants::Ui::SliderUnfreezeDelayMs, this, [this]() {
                m_ignoreSliderUpdate = false;
            });
        }
        
        // 点击后自动居中显示当前行
        m_manualScroll = false;
        ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(index.row()), QAbstractItemView::PositionAtCenter);
    }
}

/** @brief 槽：滚轮定时器超时后恢复自动跟随，将歌词列表滚动到当前播放行居中。 */
void MainWindow::onWheelTimerTimeout()
{
    m_manualScroll = false;
    if (!m_lyrics.isEmpty() && m_playerController) {
        qint64 position = m_playerController->GetPlayer()->position();
        int index = m_lrcParser->currentIndex(position);
        if (index >= 0 && index < m_lyrics.size()) {
            ui->lyricsListWidget->scrollToItem(ui->lyricsListWidget->item(index), QAbstractItemView::PositionAtCenter);
        }
    }
}

/** @brief 应用媒体加载前记录的待 seek 进度，短暂忽略 positionChanged 后清除 m_pendingSeek。 */
void MainWindow::applyPendingSeek()
{
    if (m_pendingSeek < 0 || !m_playerController) return;
    m_playerController->GetPlayer()->setPosition(m_pendingSeek);
    m_ignoreSliderUpdate = true;
    QTimer::singleShot(AppConstants::Ui::SliderUnfreezeDelayMs, this, [this]() { m_ignoreSliderUpdate = false; });
    m_pendingSeek = -1;
}

/** 媒体状态变化：LoadedMedia 时更新元数据并应用 pendingSeek；EndOfMedia 且未拖动时自动下一首。 */
void MainWindow::StatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadingMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::StalledMedia:
    case QMediaPlayer::InvalidMedia:
        break;
    case QMediaPlayer::LoadedMedia:
        UpdateMetadata();
        applyPendingSeek();
        break;
    case QMediaPlayer::BufferedMedia:
        applyPendingSeek();
        break;
    case QMediaPlayer::EndOfMedia:
        if (!m_sliderPressed) {
            m_playerController->SetAutoPlay(true);
            MusicEnd();
        }
        break;
    }
}

/** @brief 从当前播放器读取元数据，更新封面/标题/艺术家标签及歌词列表。 */
void MainWindow::UpdateMetadata()
{
    if (!m_playerController) return;
    QMediaMetaData metaData = m_playerController->GetPlayer()->metaData();

    if (!m_musicplaylist) return;
    const int currentIndex = m_playerController->GetCurrentIndex();
    if (currentIndex < 0 || currentIndex >= m_musicplaylist->Getsize()) {
        return;
    }
    const bool isCloudTrack = !m_musicplaylist->cloudSongIdAt(currentIndex).isEmpty();

    MarqueeLabel *artistlabel = ui->Controlwidget->findChild<MarqueeLabel*>("artistlabel");
    MarqueeLabel *namelabel = ui->Controlwidget->findChild<MarqueeLabel*>("namelabel");

    bool image_flag = false;

    // 云歌曲优先用播放列表缓存，避免状态变化时闪回“未知/默认图”。
    if (isCloudTrack) {
        const QString cachedTitle = m_musicplaylist->songTitleAt(currentIndex);
        const QString cachedArtist = m_musicplaylist->songArtistAt(currentIndex);
        const QPixmap cachedCover = m_musicplaylist->coverPixmapAt(currentIndex);
        if (namelabel) namelabel->setText(cachedTitle.isEmpty() ? QStringLiteral("未知曲目") : cachedTitle);
        ui->songnamelabel->setText(cachedTitle.isEmpty() ? QStringLiteral("未知曲目") : cachedTitle);
        if (artistlabel) artistlabel->setText(cachedArtist.isEmpty() ? QStringLiteral("未知艺术家") : cachedArtist);
        if (!cachedCover.isNull()) {
            ui->imagelabel->setPixmap(cachedCover);
        } else {
            image_flag = true;
        }
    } else {
        if (namelabel) namelabel->setText(QStringLiteral("未知曲目"));
        ui->songnamelabel->setText(QStringLiteral("未知曲目"));
        if (artistlabel) artistlabel->setText(QStringLiteral("未知艺术家"));
        image_flag = true;
    }

    for (auto &&[key, value] : metaData.asKeyValueRange())
    {
        if (key == QMediaMetaData::ThumbnailImage)
        {
            image_flag = false;
            ui->imagelabel->setPixmap(QPixmap::fromImage(value.value<QImage>()));
        }
        // 云来源歌曲：标题/歌手只使用播放列表缓存值，不再被媒体元数据覆盖。
        else if (!isCloudTrack && key == QMediaMetaData::ContributingArtist)
        {
            if (artistlabel) artistlabel->setText(value.toString());
        }
        else if (!isCloudTrack && key == QMediaMetaData::Title)
        {
            if (namelabel) namelabel->setText(value.toString());
            ui->songnamelabel->setText(value.toString());
        }
    }

    if(image_flag) ui->imagelabel->setPixmap(QPixmap(":/res/misaka.png"));

    // 设置歌词：根据当前播放索引加载对应 .lrc 文件
    QString filePath = m_musicplaylist->Geturl(currentIndex).toLocalFile();
    if (!filePath.isEmpty()) {
        loadLyrics(filePath);
        return;
    }

    // 云来源歌曲：每次进入播放都重新向后端请求一次歌词，避免二次播放沿用过期状态。
    const QString cloudSongId = m_musicplaylist->cloudSongIdAt(currentIndex);
    if (!cloudSongId.isEmpty() && m_cloudService) {
        const quint64 reqId = ++m_cloudRequestSeq;
        m_latestLyricsRequestId = reqId;
        m_pendingLyricsSongId = cloudSongId;
        m_cloudService->fetchLyrics(cloudSongId, reqId);
    }
}

/** 播放状态变化（预留，可按需更新 UI）。 */
void MainWindow::StateChange(QMediaPlayer::PlaybackState state)
{
    Q_UNUSED(state);
}

/** @brief 当前曲目结束，通知 PlayerController 切下一首。 */
void MainWindow::MusicEnd()
{
    if (m_playerController) {
        m_playerController->MusicEnd();
    }
}

/** @brief 窗口大小变化时重绘圆角遮罩、更新空列表 overlay 几何、播放列表位置与歌词列表高度。 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    setMask(QRegion(path.toFillPolygon().toPolygon()));

    if (m_emptyOverlayLabel) {
        // 让 overlay 始终覆盖窗口区域，文本自然居中
        m_emptyOverlayLabel->setGeometry(this->rect());
        m_emptyOverlayLabel->raise();
    }

    UpdateMusicListPosition();
    updatalyricsListWidget();
}

/** @brief 事件过滤：播放列表外点击隐藏、进度条按下/释放 seek、歌词滚轮、窗口拖拽与边缘缩放。 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    handlePlaylistAutoHideEvent(event);
    if (handleSliderEvent(obj, event)) return true;
    if (handleLyricsWheelEvent(obj, event)) return false;
    if (handleWindowDragResizeEvent(obj, event)) return true;

    // 其他事件交给父类处理
    return QMainWindow::eventFilter(obj, event);
}

bool MainWindow::handlePlaylistAutoHideEvent(QEvent *event)
{
    if (event->type() != QEvent::MouseButtonPress || !m_musicplaylist || !m_musicplaylist->isVisible()) return false;

    auto *mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->button() != Qt::LeftButton) return false;

    const QPoint globalPos = mouseEvent->globalPosition().toPoint();
    const QRect playlistGlobalRect(m_musicplaylist->mapToGlobal(QPoint(0, 0)), m_musicplaylist->size());
    if (playlistGlobalRect.contains(globalPos)) return false;

    if (ui && ui->listButton) {
        const QRect listBtnGlobalRect(ui->listButton->mapToGlobal(QPoint(0, 0)), ui->listButton->size());
        if (listBtnGlobalRect.contains(globalPos)) return false;
    }

    hidePlaylistIfVisible();
    return false;
}

bool MainWindow::handleSliderEvent(QObject *obj, QEvent *event)
{
    if (!ui || obj != ui->Slider) return false;

    if (event->type() == QEvent::MouseButtonPress) {
        m_sliderPressed = true;
        return false;
    }
    if (event->type() != QEvent::MouseButtonRelease || !m_sliderPressed) return false;

    QSlider *slider = ui->Slider;
    m_sliderPressed = false;
    const int value = slider->value();

    if (m_playerController) {
        QMediaPlayer *player = m_playerController->GetPlayer();
        const QMediaPlayer::MediaStatus status = player->mediaStatus();
        if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::NoMedia) {
            m_pendingSeek = static_cast<qint64>(value);
        } else {
            player->setPosition(static_cast<qint64>(value));
            m_ignoreSliderUpdate = true;
            QTimer::singleShot(AppConstants::Ui::SliderUnfreezeDelayMs, this, [this]() {
                m_ignoreSliderUpdate = false;
            });
        }
    }

    if (value >= slider->maximum()) MusicEnd();
    return false;
}

bool MainWindow::handleLyricsWheelEvent(QObject *obj, QEvent *event)
{
    if (!ui) return false;
    if ((obj != ui->lyricsListWidget && obj != ui->lyricsListWidget->viewport())
        || event->type() != QEvent::Wheel) {
        return false;
    }
    m_manualScroll = true;
    m_wheelTimer->start();
    return true;
}

bool MainWindow::handleWindowDragResizeEvent(QObject *obj, QEvent *event)
{
    auto *widget = qobject_cast<QWidget *>(obj);
    if (!widget) return false;
    if (widget != this && !this->isAncestorOf(widget)) return false;

    QPoint localPos;
    QPoint globalPos;
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseMove ||
        event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        globalPos = mouseEvent->globalPosition().toPoint();
        localPos = this->mapFromGlobal(globalPos);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton) return false;

        const Edge edge = getEdge(localPos);
        if (edge != NoEdge && localPos.y() >= 50) {
            startResize(edge, localPos);
            return true;
        }
        if (widget == this && localPos.y() < 50) {
            m_isDragging = true;
            m_dragStartPos = mouseEvent->globalPosition() - this->frameGeometry().topLeft();
        }
        return false;
    }

    if (event->type() == QEvent::MouseMove) {
        if (m_isResizing) {
            performResize(localPos);
            return true;
        }
        if (m_isDragging) {
            const QPointF newPos = globalPos - m_dragStartPos;
            this->move(newPos.toPoint());
            return true;
        }
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (!(mouseEvent->buttons() & Qt::LeftButton)) {
            updateCursor(getEdge(localPos));
        }
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        m_isDragging = false;
        if (m_isResizing) {
            m_isResizing = false;
            m_resizeEdge = NoEdge;
            this->unsetCursor();
        }
    }
    return false;
}

/** @brief 根据鼠标在窗口内的位置返回可拖拽边缘（左/右/上/下及其组合）。 */
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

/** @brief 根据边缘类型设置窗口光标（水平/垂直/对角箭头）。 */
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

/** @brief 开始调整大小时记录边缘、鼠标位置与当前窗口几何。 */
void MainWindow::startResize(Edge edge, const QPoint &pos)
{
    m_isResizing = true;
    m_resizeEdge = edge;
    m_resizeStartPos = pos;
    m_resizeStartGeometry = this->geometry();
}

/** @brief 根据当前鼠标位置与起始几何、边缘计算新几何并 setGeometry，保证不小于最小尺寸。 */
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

/** @brief 槽：当按下moreButton按钮时触发。 */
void MainWindow::moremenubuttonclick()
{
    if(m_moremenuwindow)
    {
        if(m_moremenuwindow->isVisible())
        {
            m_moremenuwindow->hide();
        }
        else
        {
            int target_x = ui->moreButton->x() + ui->Controlwidget->x() + ui->toolWidget->x() - 60;
            int target_y = ui->moreButton->y() + ui->Controlwidget->y() + ui->toolWidget->y() - 180;
            m_moremenuwindow->move(target_x, target_y);
            m_moremenuwindow->show();
        }
    }
}


