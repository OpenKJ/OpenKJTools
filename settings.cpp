#include "settings.h"
#include <QCoreApplication>
#include <QDir>

Settings::Settings(QObject *parent) : QObject(parent)
{
    settings = new QSettings(this);
}

QString Settings::mp3GainPath()
{
#ifdef Q_OS_WIN
    QString appDir = QCoreApplication::applicationDirPath();
    return settings->value("mp3gainPath", appDir + QDir::separator() + "mp3gain.exe").toString();
#else
    return settings->value("mp3gainPath", "/usr/bin/mp3gain").toString();
#endif
}

void Settings::setMp3GainPath(QString path)
{
    settings->setValue("mp3gainPath", path);
}

QString Settings::get7zipPath()
{
#ifdef Q_OS_WIN
    QString appDir = QCoreApplication::applicationDirPath();
    return settings->value("7zipPath", appDir + QDir::separator() + "7z.exe").toString();
#else
    return settings->value("7zipPath", "/bin/7za").toString();
#endif
}

void Settings::set7zipPath(QString path)
{
    settings->setValue("7zipPath", path);
}

bool Settings::forceReprocessing()
{
    return settings->value("forceReprocessing", false).toBool();
}

void Settings::setForceReprocessing(bool enabled)
{
    settings->setValue("forceReprocessing", enabled);
}

void Settings::setZipCompressionLevel(int level)
{
    settings->setValue("zipCompressionLevel", level);
    emit zipCompressionLevelChanged(level);
}

int Settings::zipCompressionLevel()
{
    return settings->value("zipCompressionLevel", 9).toInt();
}

void Settings::setRemoveAfterZip(bool remove)
{
    settings->setValue("removeAfterZip", remove);
}

void Settings::setRemoveAfterUnzip(bool remove)
{
    settings->setValue("removeAfterUnzip", remove);
}

void Settings::setLastPath(QString path)
{
    settings->setValue("lastPath", path);
}

bool Settings::removeAfterZip()
{
    return settings->value("removeAfterZip", false).toBool();
}

bool Settings::removeAfterUnzip()
{
    return settings->value("removeAfterUnzip", false).toBool();
}

QString Settings::lastPath()
{
    return settings->value("lastPath", QString()).toString();
}

