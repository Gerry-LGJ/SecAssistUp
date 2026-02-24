#ifndef NOTIFICATIONFORM_H
#define NOTIFICATIONFORM_H

#include <QMainWindow>
#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QDialog>
#include <QListWidget>

namespace Ui {
class NotificationForm;
}

class NotificationForm : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float opacity READ windowOpacity WRITE setWindowOpacity CONSTANT)

public:
    explicit NotificationForm(QWidget *parent = nullptr);
    explicit NotificationForm(const QString &title, const QStringList &list,int displayTime = 3000, QWidget *parent = nullptr);
    ~NotificationForm();

    void setTitle(const QString &title);
    void setStringList(const QStringList &list);
    void setDisplayTime(int msec);
    void showAtScreenCornerr();
    void showAtScreenCorner();

protected:
//     void paintEvent(QPaintEvent *event) override;
//     void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;    // 鼠标进入事件
    void leaveEvent(QEvent *event) override;    // 鼠标离开事件

private slots:
    void hideAndClose();

private:
    Ui::NotificationForm *ui;
    int                   mDisplayTime;
    QTimer               *mTimer;
    QPropertyAnimation   *mAnimation;
    QPropertyAnimation   *mFadeOut;
    QPushButton          *closeBtn;
    QListWidget          *mFiles;
    QString               mTitle;
    QStringList           mStringList;
    bool                  mIsHovered;
};

#endif // NOTIFICATIONFORM_H
