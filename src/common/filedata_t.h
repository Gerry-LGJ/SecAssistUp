#ifndef FILEDATA_T_H
#define FILEDATA_T_H

#include <QString>

typedef struct {
    bool isFile;
    QString name;
    QString path;
    QString type;
    QString mode;
    unsigned long long atime;
    unsigned long long ctime;
    unsigned long long mtime;
    int is_readable;
    int is_writeable;
    unsigned long size;
    QString size_friendly;
    QString ext;
} file_t;

typedef  struct {
    QString itemName;
    QString dirPath;
    QString operationDoneScript;
    QString lastTimeOperation;
} dir_list_t;

typedef  struct {
    QString pid;
    QString name;
    QString wdir;
    QString dspath;
    bool    rds;
    QString uspath;
    bool    rus;
    bool    fsw;
    QString lstime;
} project_info_t;

typedef struct {
    QString fid;
    QString pid;
    QString name;
    QString dir;
    bool    enable;
} uf_info_t;

#endif // FILEDATA_T_H
