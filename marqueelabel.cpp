#include "marqueelabel.h"

#include <QTimerEvent>

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

void MarqueeLabel::setScrollSpeed(int pixels)
{
    if (m_scrollSpeed != pixels) {
        m_scrollSpeed = pixels;
        emit scrollSpeedChanged(pixels);  // 发射信号
        // 如果速度变化时需要调整滚动行为，可在这里触发更新
    }
}

void MarqueeLabel::setGap(int gap)
{
    if (m_gap != gap) {
        m_gap = gap;
        emit gapChanged(gap);
        // 如果需要立即刷新显示，可调用 update()
        update();
    }
}

void MarqueeLabel::setText(const QString &text)
{
    QLabel::setText(text);
    updateScrollState();  // 文本改变后重新计算是否需要滚动
}

void MarqueeLabel::startScroll()
{
    if (!m_timer.isActive()) {
        m_timer.start(30, this);
    }
}

void MarqueeLabel::stopScroll()
{
    m_timer.stop();
    m_offset = 0;
    update();
}

void MarqueeLabel::paintEvent(QPaintEvent *event)
{
    if (!m_needScroll) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setPen(palette().windowText().color());
    painter.setFont(font());

    // int labelWidth = width();
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

void MarqueeLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    updateScrollState();
}

void MarqueeLabel::showEvent(QShowEvent *event)
{
    QLabel::showEvent(event);
    updateScrollState();
    m_offset = 0;
}

void MarqueeLabel::hideEvent(QHideEvent *event)
{
    QLabel::hideEvent(event);
    // 可选择性停止定时器以节省资源，但保留也可以
}

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
