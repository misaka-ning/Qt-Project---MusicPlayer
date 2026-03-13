#ifndef MARQUEE_LABEL_H
#define MARQUEE_LABEL_H

#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QFontMetrics>
#include <QBasicTimer>

/**
 * @brief 跑马灯效果的标签类，继承自 QLabel。
 *
 * 当文本宽度超过标签宽度时，自动向左循环滚动显示文本。
 * 支持设置滚动速度、文本间距，并提供启动/停止滚动的接口。
 */
class MarqueeLabel : public QLabel {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @param text 初始文本内容
     */
    explicit MarqueeLabel(QWidget *parent = nullptr, const QString &text = "");

    /**
     * @brief 设置滚动速度
     * @param pixels 每帧移动的像素数，正值表示向左滚动
     */
    void setScrollSpeed(int pixels);

    /**
     * @brief 获取当前滚动速度
     * @return 每帧移动的像素数
     */
    int scrollSpeed() const { return m_scrollSpeed; }

    /**
     * @brief 设置循环文本之间的间隔
     * @param gap 间隔像素数
     */
    void setGap(int gap);

    /**
     * @brief 获取当前文本间隔
     * @return 间隔像素数
     */
    int gap() const { return m_gap; }

    /**
     * @brief 手动启动滚动（通常自动启动，但可调用此方法强制启动）
     */
    void startScroll();

    /**
     * @brief 停止滚动，并重置偏移量
     */
    void stopScroll();

public slots:
    /**
     * @brief 重写 setText 槽函数，在文本改变时自动更新滚动状态
     * @param text 新文本内容
     */
    void setText(const QString &text);

signals:
    // 添加对应的 NOTIFY 信号
    void scrollSpeedChanged(int speed);
    void gapChanged(int gap);

protected:
    /**
     * @brief 重写绘制事件，实现自定义滚动绘制
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 定时器事件，用于逐帧移动偏移量
     * @param event 定时器事件
     */
    void timerEvent(QTimerEvent *event) override;

    /**
     * @brief 大小改变事件，重新判断是否需要滚动
     * @param event 大小改变事件
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief 显示事件，重置偏移量并更新滚动状态
     * @param event 显示事件
     */
    void showEvent(QShowEvent *event) override;

    /**
     * @brief 隐藏事件（可在此处停止定时器以节省资源）
     * @param event 隐藏事件
     */
    void hideEvent(QHideEvent *event) override;

private:
    /**
     * @brief 更新滚动状态：根据当前文本宽度和标签宽度判断是否需要滚动，
     *        并调整 m_needScroll 和 m_offset 的值。
     */
    void updateScrollState();

    int m_offset = 0;           ///< 当前绘制偏移量（像素），负值表示文本向左偏移
    int m_scrollSpeed = 1;      ///< 滚动速度（像素/帧）
    int m_gap = 30;             ///< 循环文本之间的间隔（像素）
    int m_textWidth = 0;        ///< 当前文本的像素宽度（使用当前字体计算）
    bool m_needScroll = false;  ///< 是否需要滚动（文本宽度 > 标签宽度）
    QBasicTimer m_timer;        ///< 用于驱动滚动的定时器
};

#endif // MARQUEE_LABEL_H
