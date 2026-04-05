#include "notificationform.h"
#include "ui_notificationform.h"

#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>

#include "../helper/filicontools.h"
#include "../helper/settingshelper.h"

NotificationForm::NotificationForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotificationForm)
{
    ui->setupUi(this);
    closeBtn = this->ui->pushButton_Close;
    mLWFiles = this->ui->listWidget_Files;

    setWindowFlags(windowFlags() |
                   Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    // setWindowOpacity(0.9);
    // setFixedSize(400, 200);

    // init timer
    mDisplayTime = 0;
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &NotificationForm::hideAndClose);

    // init animation
    mAnimation = new QPropertyAnimation(this, "opacity");
    mAnimation->setDuration(300);

    // init close button icon
    closeBtn->setFont(FilIconTools::font());
    closeBtn->setText(FilIconTools::convert(FilIcons::Type::ChromeCloseContrast));

    // connect close button slot
    connect(closeBtn, &QPushButton::clicked, this, [this] {
        hideAndClose();
    });

    // 创建窗口阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(10);
    shadowEffect->setColor(QColor(63, 63, 63, 180));
    shadowEffect->setOffset(0, 0);
    this->ui->widget_Main->setGraphicsEffect(shadowEffect);

    // 设置窗口标题
    if (mTitle.length() > 0) {
        this->ui->label_Title->setText("SecAssistUp - " + mTitle);
    } else {
        this->ui->label_Title->setText("SecAssistUp");
    }
}

NotificationForm::NotificationForm(const QString &title, const QFileInfoList &list, int displayTime, QWidget *parent) :
    mTitle(title),
    mFileInfos(list),
    mDisplayTime(displayTime),
    QWidget(parent),
    ui(new Ui::NotificationForm)
{
    ui->setupUi(this);
    closeBtn =  this->ui->pushButton_Close;
    mLWFiles = this->ui->listWidget_Files;

    // set window flags
    setWindowFlags(windowFlags() |
                   Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint);
    // set attribute
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    // set window opacity
    // setWindowOpacity(0.9);
    // set fixed size
    // setFixedSize(400, 200);

    // init timer
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &NotificationForm::hideAndClose);

    // init close button icon
    closeBtn->setFont(FilIconTools::font());
    closeBtn->setText(FilIconTools::convert(FilIcons::Type::ChromeCloseContrast));
    connect(closeBtn, &QPushButton::clicked, this, [this] {
        hideAndClose();
    });

    // Create a shadow effect
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(10);
    shadowEffect->setColor(QColor(63, 63, 63, 180));
    shadowEffect->setOffset(0, 0);
    this->ui->widget_Main->setGraphicsEffect(shadowEffect);

    // Create Window Title
    if (mTitle.length() > 0) {
        this->ui->label_Title->setText("SecAssistUp - " + mTitle);
    } else {
        this->ui->label_Title->setText("SecAssistUp");
    }
}



NotificationForm::~NotificationForm()
{
    delete ui;
}

void NotificationForm::setTitle(const QString &title)
{
    mTitle = title;
}

void NotificationForm::setStringList(const QFileInfoList &list)
{
    mFileInfos = list;
}

void NotificationForm::setDisplayTime(int msec)
{
    mDisplayTime = msec;
}

void NotificationForm::showAtScreenCornerr()
{
    qDebug() << __func__;
    // 窗口已固定大小
    // 获取屏幕右下角位置
    QScreen *screen      = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int x = screenGeometry.right() - width();
    int y = screenGeometry.bottom() - height();

    // 初始位置在屏幕（右侧）
    move(screenGeometry.right(), y);
    show();

    // 启动进入动画（从右侧滑入）
    mAnimation->setStartValue(windowOpacity());
    mAnimation->setEndValue(0.9);
    mAnimation->setEasingCurve(QEasingCurve::OutCubic);
    QPoint startPos = pos();
    QPoint endPos   = QPoint(x, y);

    // 使用单独的动画来移动位置
    QPropertyAnimation *moveAnimation = new QPropertyAnimation(this, "pos");
    moveAnimation->setDuration(300);
    moveAnimation->setStartValue(startPos);
    moveAnimation->setEndValue(endPos);
    moveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 并行执行两个动画
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(mAnimation);
    group->addAnimation(moveAnimation);
    group->start();

    // 启动自动关闭定时器
    if (mDisplayTime > 0) {
        mTimer->start(mDisplayTime);
    }
}

void NotificationForm::showAtScreenCorner()
{
    qDebug() << __func__ << "width:" << width() << "height:" << height();

    // Check if bubble notifications are enabled.
    if (!SettingsHelper::getInstance()->getAppNotifyBubble()) {
        qDebug() << __func__ << "Notify Bubble Enable is false.";
        deleteLater();
        return ;
    }

    // add list data
    QStringList mStringList;
    mLWFiles->clear();
    for (int i = 0; i < mFileInfos.size(); ++i) {
        mStringList.append(mFileInfos.at(i).fileName());
    }
    mLWFiles->addItems(mStringList);

    // Obtain the position at the bottom right corner of the screen.
    QScreen *screen      = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // Calculate the final position for window drawing.
    int x = screenGeometry.right() - width();
    int y = screenGeometry.bottom() - height();

    // Set the drawing position when initializing the window.
    move(screenGeometry.right(), y);
    show();

    // Start the animation (slide in from the right).
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(windowOpacity());
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // Use separate animations to move the position.
    QPoint startPos                   = pos();
    QPoint endPos                     = QPoint(x, y);
    QPropertyAnimation *moveAnimation = new QPropertyAnimation(this, "pos");
    moveAnimation->setDuration(300);
    moveAnimation->setStartValue(startPos);
    moveAnimation->setEndValue(endPos);
    moveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Execute two animations in parallel.
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    group->addAnimation(animation);
    group->addAnimation(moveAnimation);
    group->start();

    // Start the automatic shut-off timer.
    if (mDisplayTime > 0) {
        mTimer->start(mDisplayTime);
    }
}

void NotificationForm::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    qDebug() << __func__;

    mIsHovered = true;

    // stop timer
    if (mTimer->isActive()) {
        mTimer->stop();
        qDebug() << __func__ << "Timer stoped - mouse entered";
    }
}

void NotificationForm::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    qDebug() << __func__;

    mIsHovered = false;

    hideAndClose();
}

void NotificationForm::hideAndClose()
{
    qDebug() << __func__;
    mTimer->stop();

    // 淡出动画
    mFadeOut = new QPropertyAnimation(this, "opacity");
    mFadeOut->setDuration(300);
    mFadeOut->setStartValue(windowOpacity());
    mFadeOut->setEndValue(0.0);
    mFadeOut->setEasingCurve(QEasingCurve::InCubic);

    connect(mFadeOut, &QPropertyAnimation::finished, this, [this] () {
        close();
        deleteLater();
    });

    mFadeOut->start();
}
