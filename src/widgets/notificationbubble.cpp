// notificationbubble.cpp
#include "notificationbubble.h"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QParallelAnimationGroup>

NotificationBubble::NotificationBubble(const QString &message, int displayTime, QWidget *parent)
    : QMainWindow(parent), m_message(message), m_displayTime(displayTime)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint |    // 无边框
                   Qt::Tool |                   // 工具窗口（不会获取焦点）
                   Qt::WindowStaysOnTopHint);   // 置顶

    setAttribute(Qt::WA_TranslucentBackground); // 透明背景
    setAttribute(Qt::WA_ShowWithoutActivating); // 显示时不激活
    setWindowOpacity(0.9);                      // 设置透明度

    // 设置样式
    setStyleSheet("NotificationBubble { background-color: #333333; border-radius: 8px; }");

    // 初始化定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &NotificationBubble::hideAndClose);

    // 初始化动画
    m_animation = new QPropertyAnimation(this, "opacity");
    m_animation->setDuration(300);
}

void NotificationBubble::showAtScreenCorner()
{
    // 计算窗口大小
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(m_message) + 80;
    int textHeight = fm.height() + 40;

    setFixedSize(qMin(textWidth, 400), qMax(textHeight, 200));

    // 获取屏幕右下角位置
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int x = screenGeometry.right() - width();
    int y = screenGeometry.bottom() - height();

    // 初始位置在屏幕外（右侧）
    move(screenGeometry.right(), y);
    show();

    // 启动进入动画（从右侧滑入）
    m_animation->setStartValue(windowOpacity());
    m_animation->setEndValue(0.9);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    QPoint startPos = pos();
    QPoint endPos = QPoint(x, y);

    // 使用单独的动画来移动位置
    QPropertyAnimation *moveAnimation = new QPropertyAnimation(this, "pos");
    moveAnimation->setDuration(300);
    moveAnimation->setStartValue(startPos);
    moveAnimation->setEndValue(endPos);
    moveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 并行执行两个动画
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(m_animation);
    group->addAnimation(moveAnimation);
    group->start();

    // 启动自动关闭定时器
    m_timer->start(m_displayTime);
}

void NotificationBubble::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.setBrush(QColor(51, 51, 51, 230)); // 深灰色背景
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);

    // 绘制文字
    painter.setPen(Qt::white);
    painter.setFont(font());
    QTextOption option;
    option.setAlignment(Qt::AlignCenter);
    painter.drawText(rect().adjusted(10, 10, -10, -10), m_message, option);

    // 绘制关闭按钮
    painter.setPen(QPen(Qt::white, 2));
    int closeSize = 20;
    QRect closeRect(width() - closeSize - 10, 10, closeSize, closeSize);

    painter.drawLine(closeRect.topLeft(), closeRect.bottomRight());
    painter.drawLine(closeRect.topRight(), closeRect.bottomLeft());
}

void NotificationBubble::mousePressEvent(QMouseEvent *event)
{
    // 点击关闭按钮区域
    int closeSize = 20;
    QRect closeRect(width() - closeSize - 10, 10, closeSize, closeSize);

    if (closeRect.contains(event->pos())) {
        hideAndClose();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void NotificationBubble::hideAndClose()
{
    m_timer->stop();

    // 淡出动画
    QPropertyAnimation *fadeOut = new QPropertyAnimation(this, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(windowOpacity());
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
        close();
        deleteLater();
    });

    fadeOut->start();
}
