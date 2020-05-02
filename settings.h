#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT
private:
    QSettings *settings;
public:
    explicit Settings(QObject *parent = 0);
    QString mp3GainPath();
    void setMp3GainPath(QString path);
    QString get7zipPath();
    void set7zipPath(QString path);
    QString ffmpegPath();
    void setFfmpegPath(QString path);
    bool forceReprocessing();
    void setForceReprocessing(bool enabled);
    int zipCompressionLevel();
    bool removeAfterZip();
    bool removeAfterUnzip();
    QString lastPath();

signals:
    void zipCompressionLevelChanged(int level);

public slots:
    void setZipCompressionLevel(int level);
    void setRemoveAfterZip(bool remove);
    void setRemoveAfterUnzip(bool remove);
    void setLastPath(QString path);

};

#endif // SETTINGS_H
