#include "versionclass.h"

Version::Version()
    : mMajor(0), mMinor(0), mPatch(0)
{}

Version::Version(unsigned int major, unsigned int minor, unsigned int patch)
    : mMajor(major), mMinor(minor), mPatch(patch)
{}

Version::Version(const QString& versionStr)
{
    parse(versionStr);
}

bool Version::isValid() const
{
    return ((mMajor == 0) && (mMinor == 0) && (mPatch == 0));
}

bool Version::parse(const QString& versionStr)
{
    mMajor = 0;
    mMinor = 0;
    mPatch = 0;

    QStringList parts = versionStr.split('.');
    if (parts.size() != 3) {
        return false;
    }
    bool ok1, ok2, ok3;
    unsigned int major = 0, minor = 0, patch = 0;

    major = parts[0].toUInt(&ok1);
    minor = parts[1].toUInt(&ok2);
    patch = parts[2].toUInt(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        return false;
    }
    mMajor = major;
    mMinor = minor;
    mPatch = patch;
    return true;
}

void Version::setVersion(unsigned int major, unsigned int minor, unsigned int patch)
{
    mMajor = major;
    mMinor = minor;
    mPatch = patch;
}

static Version fromString(const QString& versionStr)
{
    QStringList parts = versionStr.split('.');
    if (parts.size() != 3) {
        return Version();
    }
    bool ok1, ok2, ok3;
    unsigned int major = 0, minor = 0, patch = 0;

    major = parts[0].toUInt(&ok1);
    minor = parts[1].toUInt(&ok2);
    patch = parts[2].toUInt(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        return Version();
    }
    return Version(major, minor, patch);
}

bool Version::operator==(const Version& other) const
{
    return mMajor == other.mMajor &&
           mMinor == other.mMinor &&
           mPatch == other.mPatch;
}

bool Version::operator != (const Version& other) const
{
    return !(*this == other);
}

bool Version::operator <  (const Version& other) const
{
    if (mMajor != other.mMajor)
        return mMajor < other.mMajor;
    if (mMinor != other.mMinor)
        return mMinor < other.mMinor;
    return mPatch < other.mPatch;
}

bool Version::operator <= (const Version& other) const
{
    return *this < other || *this == other;
}

bool Version::operator >  (const Version& other) const
{
    return other < *this;
}

bool Version::operator >= (const Version& other) const
{
    return other <= *this;
}

QString Version::toString() const
{
    return QString("%1.%2.%3").arg(mMajor).arg(mMinor).arg(mPatch);
}

std::ostream& operator << (std::ostream& os, const Version& v)
{
    os << v.major() << "." << v.minor() << "." << v.patch();
    return os;
}

QDebug operator << (QDebug debug, const Version& v)
{
    QDebugStateSaver saver(debug);
    debug.noquote() << v.major() << "." << v.minor() << "." << v.patch();
    return debug;
}
