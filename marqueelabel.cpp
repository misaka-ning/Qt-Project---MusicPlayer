#include "marqueelabel.h"

#include <QTimerEvent>

/** @brief 设置文本、不换行、尺寸策略，更新滚动状态并启动定时器。 */
MarqueeLabel::MarqueeLabel(QWidget *parent, const QString &text)
    : QLabel(parent)
{
    setText(text);
    setWordWrap(false);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    // 首次更新滚动状态（如果文本已设置）
    updateScrollState();
    // 启动定时器（即使当前不需要滚动，也不会做无用功，因为 m_needScroll 为 false）
    startScroll();
}

/** @brief 设置每帧滚动像素并发射 scrollSpeedChanged。 */
void MarqueeLabel::setScrollSpeed(int pixels)
{
    if (m_scrollSpeed != pixels) {
        m_scrollSpeed = pixels;
        emit scrollSpeedChanged(pixels);  // 发射信号
        // 如果速度变化时需要调整滚动行为，可在这里触发更新
    }
}

/** @brief 设置循环间隔并发射 gapChanged，调用 update。 */
void MarqueeLabel::setGap(int gap)
{
    if (m_gap != gap) {
        m_gap = gap;
        emit gapChanged(gap);
        // 如果需要立即刷新显示，可调用 update()
        update();
    }
}

/** @brief 设置文本并重新计算是否需要滚动。 */
void MarqueeLabel::setText(const QString &text)
{
    QLabel::setText(text);
    updateScrollState();
}

/** @brief 若定时器未运行则启动（30ms 间隔）。 */
void MarqueeLabel::startScroll()
{
    if (!m_timer.isActive()) {
        m_timer.start(30, this);
    }
}

/** @brief 停止定时器、偏移归零并重绘。 */
void MarqueeLabel::stopScroll()
{
    m_timer.stop();
    m_offset = 0;
    update();
}

/** @brief 需要滚动时自绘两段文本（首尾相接），否则调用基类。 */
void MarqueeLabel::paintEvent(QPaintEvent *event)
{
    if (!m_needScroll) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setPen(palette().windowText().color());
    painter.setFont(font());
    int textHeight = height();
    QFontMetrics fm = painter.fontMetrics();
    int textY = (textHeight - fm.height()) / 2 + fm.ascent();

    // 计算两个副本的位置
    int firstX = m_offset;
    int secondX = m_offset + m_textWidth + m_gap;

    // 绘制两个副本（超出区域的会被自动裁剪）
    painter.drawText(firstX, textY, text());
    painter.drawText(secondX, textY, text());
}

/** @brief 定时器到时且需要滚动时更新 m_offset，超出一周期则回绕。 */
void MarqueeLabel::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer.timerId() && m_needScroll) {
        m_offset -= m_scrollSpeed;
        int period = m_textWidth + m_gap;
        if (m_offset < -period) {
            m_offset += period;
        }
        update();
    }
}

/** @brief 大小变化时重新判断是否需要滚动。 */
void MarqueeLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    updateScrollState();
}

/** @brief 显示时重置偏移并更新滚动状态。 */
void MarqueeLabel::showEvent(QShowEvent *event)
{
    QLabel::showEvent(event);
    updateScrollState();
    m_offset = 0;
}

/** @brief 隐藏时交给基类处理。 */
void MarqueeLabel::hideEvent(QHideEvent *event)
{
    QLabel::hideEvent(event);
}

/** @brief 根据文本宽度与标签宽度更新 m_needScroll，需要时重置 m_offset 或停止绘制偏移。 */
void MarqueeLabel::updateScrollState()
{
    QFontMetrics fm(font());
    m_textWidth = fm.horizontalAdvance(text());
    bool need = (m_textWidth > width());
    if (need != m_needScroll) {
        m_needScroll = need;
        if (need) {
            // 开始滚动前重置偏移量（可选）
            m_offset = 0;
            // 如果定时器未运行，启动它（但 startScroll 已在构造函数中调用，所以通常已运行）
        } else {
            // 停止滚动并重绘为普通状态
            m_offset = 0;
            update();
        }
    }
}
