#ifndef ZIPHANDLER_H
#define ZIPHANDLER_H

#include <QObject>
#include <QDir>
#include <QFile>
#include "miniz.h"
#include "settings.h"

class ZipHandler : public QObject
{
    Q_OBJECT
private:
    Settings *settings;
    QString m_zipFile;
    QStringList audioExtensions;
    void loadExtensions();
    void initializeZipFile();
    void resetVars();
    bool m_zipFileValid;
    bool m_compressionSupportedAudio;
    bool m_compressionSupportedCdg;
    bool m_audioFileFound;
    bool m_cdgFileFound;
    mz_zip_archive *m_archive;
    QString m_audioExt;
    QString m_audioFileName;
    QString m_cdgFileName;
    int m_cdgSize;
    int m_audioSize;
    int m_compressionLevel;
    bool m_compressFilesWithZipFn;
    bool m_rgMarkerFileFound;
    QFile *qzFile;
    FILE *czFile;

public:
    explicit ZipHandler(QString zipFile, QObject *parent = 0);
    explicit ZipHandler(QObject *parent = 0);
    //~KhZip2();
    bool extractAudio(QDir destDir);
    bool extractCdg(QDir destDir);
    QString zipFile() const;
    void setZipFile(const QString &zipFile);
    int getSongDuration();
    bool createZip(QString zipFile, QString cdgFile, QString audioFile, bool addRgMarker = false);
    bool isValidZipfile();
    bool containsRGMarkerFile();
    bool compressionSupported();
    bool containsAudioFile();
    bool containsCdgFile();
    void setCompressionLevel(int level = 9);
    void setReplaceFnsWithZipFn(bool replace = true);
    bool extractUsing7z(QString fileName, QString destDir);
    bool reZipUnsupported(QString filePath);
    QString getAudioExt();
    QStringList getSupportedAudioExts();
    void close();


signals:

public slots:

};

#endif // ZIPHANDLER_H
