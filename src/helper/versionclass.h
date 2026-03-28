#ifndef VERSIONCLASS_H
#define VERSIONCLASS_H

#include <QString>
#include <QStringList>
#include <QDebug>

class Version {
public:

    Version();
    Version(unsigned int major, unsigned int minor, unsigned int patch);
    explicit Version(const QString& versionStr);

    bool isValid() const;
    bool parse(const QString& versionStr);
    void setVersion(unsigned int major, unsigned int minor, unsigned int patch);

    unsigned int major() const { return mMajor; }
    unsigned int minor() const { return mMinor; }
    unsigned int patch() const { return mPatch; }

    bool operator == (const Version& other) const;
    bool operator != (const Version& other) const;
    bool operator <  (const Version& other) const;
    bool operator <= (const Version& other) const;
    bool operator >  (const Version& other) const;
    bool operator >= (const Version& other) const;
    QString toString() const;

private:
    unsigned int mMajor;
    unsigned int mMinor;
    unsigned int mPatch;
};
inline std::ostream& operator << (std::ostream& os, const Version& v);
inline QDebug operator << (QDebug debug, const Version& v);

#endif // VERSIONCLASS_H
