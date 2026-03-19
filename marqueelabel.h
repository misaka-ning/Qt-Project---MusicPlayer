#ifndef MARQUEE_LABEL_H
#define MARQUEE_LABEL_H

#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QFontMetrics>
#include <QBasicTimer>

/** 跑马灯标签：文本宽度超过标签宽度时向左循环滚动显示。 */
class MarqueeLabel : public QLabel {
    Q_OBJECT
public:
    explicit MarqueeLabel(QWidget *parent = nullptr, const QString &text = "");
    void setScrollSpeed(int pixels);   // 每帧移动像素数，正值向左
    int scrollSpeed() const { return m_scrollSpeed; }
    void setGap(int gap);               // 循环文本间隔（像素）
    int gap() const { return m_gap; }
    void startScroll();                 // 启动滚动定时器
    void stopScroll();                  // 停止滚动并重置偏移

public slots:
    void setText(const QString &text);  // 重写：设置文本后更新滚动状态

signals:
    void scrollSpeedChanged(int speed);
    void gapChanged(int gap);

protected:
    void paintEvent(QPaintEvent *event) override;    // 自定义绘制实现滚动
    void timerEvent(QTimerEvent *event) override;     // 逐帧更新偏移
    void resizeEvent(QResizeEvent *event) override;   // 尺寸变化时更新是否需要滚动
    void showEvent(QShowEvent *event) override;       // 显示时重置偏移与滚动状态
    void hideEvent(QHideEvent *event) override;

private:
    void updateScrollState();   // 根据文本宽度与标签宽度设置 m_needScroll 与 m_offset

    int m_offset = 0;
    int m_scrollSpeed = 1;
    int m_gap = 30;
    int m_textWidth = 0;
    bool m_needScroll = false;
    QBasicTimer m_timer;
};

#endif // MARQUEE_LABEL_H
