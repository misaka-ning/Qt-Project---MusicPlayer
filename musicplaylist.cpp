#include "musicplaylist.h"
#include "ui_musicplaylist.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
 
#include <QScrollArea>
#include <QResizeEvent>

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
    setAutoFillBackground(false);

    // 关键：QScrollArea/viewport/contents 默认会自己填充背景，导致父控件（MusicPlaylist）的 QSS 背景被盖住
    // 这里把它们设为“可被 QSS 绘制 + 不主动填充背景”，再配合 style.qss 里的透明背景规则即可生效
    ui->scrollArea->setAttribute(Qt::WA_StyledBackground, true);
    ui->scrollArea->setAutoFillBackground(false);
    if (ui->scrollArea->viewport()) {
        ui->scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
        ui->scrollArea->viewport()->setAutoFillBackground(false);
    }
    ui->scrollAreaWidgetContents->setAttribute(Qt::WA_StyledBackground, true);
    ui->scrollAreaWidgetContents->setAutoFillBackground(false);

    // 获取内部容器的布局
    m_layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!m_layout) {
        // 如果 UI 中未正确设置布局，则创建一个（安全后备）
        m_layout = new QVBoxLayout(ui->scrollAreaWidgetContents);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
    }

    // 初始化淡入淡出效果（作用于整个 MusicPlaylist）
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    m_opacityAnim = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_posAnim = new QPropertyAnimation(this, "pos", this);

    m_animGroup = new QParallelAnimationGroup(this);
    m_animGroup->addAnimation(m_opacityAnim);
    m_animGroup->addAnimation(m_posAnim);

    connect(m_animGroup, &QParallelAnimationGroup::finished, this, [this]() {
        m_isAnimating = false;
        if (!m_shouldBeVisible) {
            QWidget::hide();
        } else {
            // 确保收敛到目标位置/完全可见
            move(m_targetPos);
            if (m_opacityEffect) m_opacityEffect->setOpacity(1.0);
        }
    });

    // 空列表占位文案（覆盖在 scrollArea 之上）
    m_emptyLabel = new QLabel(this);
    m_emptyLabel->setText(QStringLiteral("当前没有加载的音乐"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_emptyLabel->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: rgba(255, 255, 255, 180);"
        "  font-size: 16px;"
        "  background: transparent;"
        "}"
    ));
    updateEmptyStateUi();
}

MusicPlaylist::~MusicPlaylist()
{
    delete ui;
}

int MusicPlaylist::slideOffsetPx() const
{
    // 用自身宽度的一部分做滑入距离，避免硬编码；同时下限保证“看得出位移”
    return qMax(120, width() / 3);
}

int MusicPlaylist::animDurationMs() const
{
    return 180;
}

void MusicPlaylist::setTargetPos(const QPoint& p)
{
    m_targetPos = p;
    if (isVisible() && !m_isAnimating) {
        move(m_targetPos);
    }
}

QPoint MusicPlaylist::targetPos() const
{
    return m_targetPos;
}

bool MusicPlaylist::isAnimating() const
{
    return m_isAnimating;
}

void MusicPlaylist::showAnimated()
{
    m_shouldBeVisible = true;

    if (!m_opacityEffect || !m_animGroup || !m_opacityAnim || !m_posAnim) {
        move(m_targetPos);
        show();
        return;
    }

    m_animGroup->stop();
    m_isAnimating = true;

    const int offset = slideOffsetPx();
    const QPoint startPos = m_targetPos + QPoint(offset, 0);

    // 若当前不可见：从“右侧 + 透明”开始；若正在隐藏中：从当前状态接续
    if (!isVisible()) {
        move(startPos);
        m_opacityEffect->setOpacity(0.0);
        show();
    }

    m_opacityAnim->setDuration(animDurationMs());
    m_opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_opacityAnim->setStartValue(m_opacityEffect->opacity());
    m_opacityAnim->setEndValue(1.0);

    m_posAnim->setDuration(animDurationMs());
    m_posAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_posAnim->setStartValue(pos());
    m_posAnim->setEndValue(m_targetPos);

    m_animGroup->start();
}

void MusicPlaylist::hideAnimated()
{
    m_shouldBeVisible = false;

    if (!m_opacityEffect || !m_animGroup || !m_opacityAnim || !m_posAnim) {
        hide();
        return;
    }

    if (!isVisible()) {
        return;
    }

    m_animGroup->stop();
    m_isAnimating = true;

    const int offset = slideOffsetPx();
    const QPoint endPos = m_targetPos + QPoint(offset, 0);

    m_opacityAnim->setDuration(animDurationMs());
    m_opacityAnim->setEasingCurve(QEasingCurve::InCubic);
    m_opacityAnim->setStartValue(m_opacityEffect->opacity());
    m_opacityAnim->setEndValue(0.0);

    m_posAnim->setDuration(animDurationMs());
    m_posAnim->setEasingCurve(QEasingCurve::InCubic);
    m_posAnim->setStartValue(pos());
    m_posAnim->setEndValue(endPos);

    m_animGroup->start();
}

void MusicPlaylist::AppendMusic(QPixmap pix, QUrl url, QString name, QString artist)
{
    const bool wasEmpty = m_musiclist.isEmpty();

    SongUnit *unit = new SongUnit(m_musiclist.size(), pix, url, name, artist, this);
    // 添加到布局（插入到弹簧之前）
    if (m_layout->count() > 0) {
        // 如果布局中有弹簧，插入到最后一个元素（弹簧）之前，确保弹簧始终在最底部
        m_layout->insertWidget(m_layout->count() - 1, unit);
    } else {
        m_layout->addWidget(unit);
    }
    unit->show();

    m_musiclist.append(unit);

    connect(unit, &SongUnit::ChooseMusic, this, [this](int id){emit ChooseMusicpass(id);});
    updateEmptyStateUi();
    emit songsChanged(m_musiclist.size());
    if (wasEmpty) {
        emit hasSongsChanged(true);
    }
}

int MusicPlaylist::appendSong(const QPixmap& pix, const QUrl& url, const QString& name, const QString& artist)
{
    const int index = m_musiclist.size();
    AppendMusic(pix, url, name, artist);
    return index;
}

bool MusicPlaylist::removeSongAt(int index)
{
    if (index < 0 || index >= m_musiclist.size()) return false;

    const bool wasNonEmpty = !m_musiclist.isEmpty();

    SongUnit* unit = m_musiclist[index];
    m_musiclist.removeAt(index);

    if (unit) {
        if (m_layout) m_layout->removeWidget(unit);
        unit->hide();
        unit->deleteLater();
    }

    // Re-pack ids so that SongUnit::ChooseMusic(id) still maps to current index.
    for (int i = index; i < m_musiclist.size(); ++i) {
        if (m_musiclist[i]) m_musiclist[i]->SetId(i);
    }

    updateEmptyStateUi();
    emit songsChanged(m_musiclist.size());

    if (wasNonEmpty && m_musiclist.isEmpty()) {
        emit hasSongsChanged(false);
    }

    return true;
}

void MusicPlaylist::clearSongs()
{
    const bool wasNonEmpty = !m_musiclist.isEmpty();

    for (SongUnit* unit : std::as_const(m_musiclist)) {
        if (!unit) continue;
        if (m_layout) m_layout->removeWidget(unit);
        unit->hide();
        unit->deleteLater();
    }
    m_musiclist.clear();

    updateEmptyStateUi();
    emit songsChanged(0);
    if (wasNonEmpty) {
        emit hasSongsChanged(false);
    }
}

bool MusicPlaylist::isempty()
{
    return m_musiclist.empty();
}

QUrl MusicPlaylist::Geturl(const int n)
{
    if (n < 0 || n >= m_musiclist.size()) return QUrl();
    SongUnit *unit = m_musiclist[n];
    if (!unit) return QUrl();
    return unit->Geturl();
}

int MusicPlaylist::Getsize()
{
    return m_musiclist.size();
}

void MusicPlaylist::updateItem(int idx, QPixmap image, QString name, QString artist)
{
    if (idx < 0 || idx >= m_musiclist.size()) return;
    m_musiclist[idx]->SetPixmap(image);
    m_musiclist[idx]->SetName(name);
    m_musiclist[idx]->SetArtist(artist);
    m_musiclist[idx]->UiUpdate();
}

void MusicPlaylist::updateEmptyStateUi()
{
    if (!m_emptyLabel) return;

    const bool empty = m_musiclist.isEmpty();
    m_emptyLabel->setVisible(empty);
    m_emptyLabel->raise();
    m_emptyLabel->setGeometry(rect());
}

void MusicPlaylist::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateEmptyStateUi();
}
