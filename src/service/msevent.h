#pragma once

#include <QEvent>
#include <QVariant>

class MSEvent: public QEvent
{
public:
    static const QEvent::Type MSEVENT_BASE_TYPE = static_cast<QEvent::Type>(QEvent::User + 0x100);
    enum {
        LOGIN_STR_MAX_LENGTH = 255,
    };
    enum {
        EVENT_TYPE_BASE_REQ = 0x0100,
        EVENT_TYPE_LOGIN_REQ,
        EVENT_TYPE_LOGOUT_REQ,
        EVENT_TYPE_REFRESH_REQ,
        EVENT_TYPE_ENTRY_FOLDER_REQ,
        EVENT_TYPE_RETURN_PARENT_DIR_REQ,
        EVENT_TYPE_DOWNLOAD_REQ,
        EVENT_TYPE_UPLOAD_REQ
    };
    enum {
        EVENT_TYPE_BASE_CFM = 0x0200,
        EVENT_TYPE_LOGIN_CFM,
        EVENT_TYPE_LOGOUT_CFM,
        EVENT_TYPE_REFRESH_CFM,
        EVENT_TYPE_DOWNLOAD_CFM,
        EVENT_TYPE_UPLOAD_CFM
    };
    enum {
        EVENT_TYPE_BASE_IND = 0x0300,
        EVENT_TYPE_LOGIN_STATE_CHANGED_IND,
        EVENT_TYPE_REFRESH_IND,
        EVENT_TYPE_FILE_CHANGED_IND
    };
    enum {
        RESULT_CODE_SUCCESS = 0x0000,
        RESULT_CODE_FAIL,
        RESULT_CODE_NETWORK_ONERROR,
    };
    MSEvent(QObject *sender, uint16_t event)
        : QEvent(MSEVENT_BASE_TYPE), mSender(sender), mEventType(event) { }
    void setData(QVariant data) {
        mData = data;
    }
    QVariant getData() {
        return mData;
    }
    uint16_t getMSEventType() {
        return mEventType;
    }
    QObject *getSender() {
        return mSender;
    }
    void *getMsg() { return msg; }
    // LOGIN MESSAGE STRUCT
    typedef struct {
        char username[LOGIN_STR_MAX_LENGTH - 1];
        char password[LOGIN_STR_MAX_LENGTH - 1];
        char weburl  [LOGIN_STR_MAX_LENGTH - 1];
    } event_type_login_req_t;
    typedef struct {
        uint16_t resultCode;
        uint16_t resultSupplier;
    } event_type_login_cfm_t;
    // LOGOUT MESSAGE STRUCT
    typedef struct {} event_type_logout_req_t;
    typedef struct {} event_type_logout_cfm_t;
    // REFRESH MESSAGE STRUCT
    typedef struct {} event_type_refresh_req_t;
    typedef struct {} event_type_refresh_cfm_t;
    // DOWNLOAD MESSAGE STRUCT
    typedef struct {} event_type_download_req_t;
    typedef struct {} event_type_download_cfm_t;
    // UPLOAD MESSAGE STRUCT
    typedef struct {} event_type_upload_req_t;
    typedef struct {} event_type_upload_cfm_t;
    union {
        event_type_logout_req_t event_type_logout_req;
        event_type_logout_cfm_t event_type_logout_cfm;
        event_type_refresh_req_t event_type_refresh_req;
        event_type_refresh_cfm_t event_type_refresh_cfm;
        event_type_download_req_t event_type_download_req;
        event_type_download_cfm_t event_type_download_cfm;
        event_type_upload_req_t event_type_upload_req;
        event_type_upload_cfm_t event_type_upload_cfm;
    } event_type_u;

private:
    uint16_t mEventType;
    QObject *mSender;
    QVariant mData;
    void    *msg;
};
