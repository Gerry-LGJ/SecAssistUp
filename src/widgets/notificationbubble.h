// notificationbubble.h
#ifndef NOTIFICATIONBUBBLE_H
#define NOTIFICATIONBUBBLE_H

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QMainWindow>

class NotificationBubble : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(float opacity READ windowOpacity WRITE setWindowOpacity)

public:
    explicit NotificationBubble(const QString &message, int displayTime = 3000, QWidget *parent = nullptr);
    void showAtScreenCorner();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void hideAndClose();

private:
    QString m_message;
    int m_displayTime;
    QTimer *m_timer;
    QPropertyAnimation *m_animation;
};

#endif // NOTIFICATIONBUBBLE_H
