#ifndef OTHERSERVICE_H
#define OTHERSERVICE_H

#include <QObject>

#include "../helper/network.h"
#include "../helper/settingshelper.h"
#include "../common/singleton.h"
#include "../window/loginwindow.h"
#include "../window/mainwindow.h"
#include "msevent.h"

typedef struct {
    QString type;
    QString path;
} del_info_t;

typedef struct {
    QString type;
    QString path;
} cut_info_t;


class OtherService : public QObject
{
    Q_OBJECT
public:
    enum {
        STATE_TYPE_IDLE,
        STATE_TYPE_DELETING,
        STATE_TYPE_RENAMING,
        STATE_TYPE_CUTTING,
        STATE_TYPE_PASTING
    };
    SINGLETON(OtherService)
    void init();


protected:
    bool event(QEvent *e);


private:
    uint16_t                  mState;
    bool                      mDeleting, mRenaming, mCutting, mPasting;

    // inst
    QObject                  *mSender;
    SettingsHelper           *mSettings;
    Network                  *mNetwork;
    NetworkCallable          *mDeleteCallable;
    NetworkCallable          *mRenameCallable;
    NetworkCallable          *mCutCallable;
    NetworkCallable          *mPasteCallable;
    // win inst
    LoginWindow              *mLoginWindow;
    MainWindow               *mMainWindow;
    // delete part
    QList<del_info_t>         mDeleteList;
    // rename part
    // cut    part
    // paste  part

    explicit OtherService(QObject *parent = nullptr);
    void initCallable();
    void initDeleteCallable();
    void initRenameCallable();
    void initCutCallable();
    void initPasteCallable();
    bool otherServiceEventHandler(MSEvent *e);
    void transition(uint16_t state);

    bool del(QList<del_info_t> &list);
    bool rename(const QString &path, const QString &rname_to);
    bool cut(const QList<cut_info_t> &list);
    bool paste(const QString &path);

    // tools
    static bool returnLoginPage(NetworkCallable *callable, QString &result);

    void sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);

signals:
};


Q_DECLARE_METATYPE(del_info_t)
Q_DECLARE_METATYPE(QList<del_info_t>)
Q_DECLARE_METATYPE(cut_info_t)
Q_DECLARE_METATYPE(QList<cut_info_t>)

#endif // OTHERSERVICE_H
