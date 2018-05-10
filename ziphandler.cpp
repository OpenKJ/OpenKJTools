#include "ziphandler.h"
#include <QDebug>
#include <QTemporaryDir>
#include <fstream>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QDirIterator>

#if defined(__GNUC__)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#endif

void ZipHandler::resetVars()
{
    m_zipFileValid = false;
    m_compressionSupportedAudio = false;
    m_compressionSupportedCdg = false;
    m_audioFileFound = false;
    m_cdgFileFound = false;
    m_audioSize = 0;
    m_cdgSize = 0;
    m_compressionLevel = 9;
    m_compressFilesWithZipFn = true;
    m_rgMarkerFileFound = false;
}

bool ZipHandler::extractUsing7z(QString fileName, QString destDir)
{
    QTemporaryDir tmpDir;
    QString tmpZipPath = tmpDir.path() + QDir::separator() + "tmp.zip";
    if (!QFile::copy(fileName, tmpZipPath))
    {
        qWarning() << "error copying zip";
        return false;
    }
    QProcess *process = new QProcess(this);
    QString ext7zPath = "/bin/7za";
    QStringList arguments;
    arguments << "e";
    arguments << "-y";
    arguments << "-o" + destDir;
    arguments << tmpZipPath;
    process->start(settings->get7zipPath(), arguments);
    process->waitForFinished();
    qWarning() << process->readAllStandardOutput();
    qWarning() << process->readAllStandardError();
    if (process->exitCode() != 0)
        return false;
    else
        return true;
}

bool ZipHandler::reZipUnsupported(QString filePath)
{
    QString zipFileName = QFileInfo(filePath).fileName();
    QTemporaryDir tmpDir;
    if (!extractUsing7z(filePath, tmpDir.path()))
    {
        qWarning() << "Failed to extract files";
        return false;
    }
    QString audioFile;
    QString cdgFile;
    QDirIterator iterator(tmpDir.path(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString fn = iterator.filePath();
            if (fn.endsWith(".cdg", Qt::CaseInsensitive))
            {
                cdgFile = fn;
            }
            else
            {
                for (int e=0; e < audioExtensions.size(); e++)
                {
                    if (fn.endsWith(audioExtensions.at(e), Qt::CaseInsensitive))
                    {
                        audioFile = fn;
                    }
                }
            }
        }
    }
    if (audioFile == "" || cdgFile == "")
    {
        qWarning() << "File missing either mp3 of cdg file, bailing out";
        return false;
    }
    createZip(tmpDir.path() + QDir::separator() + zipFileName, cdgFile, audioFile);
    if (!QFile::rename(filePath, filePath + ".tmp"))
    {
        qWarning() << "Unable to move original to temporary file, bailing out";
        qWarning() << "rename(" << filePath << ", " << filePath << ".tmp)";
        return false;
    }
    if (!QFile::copy(tmpDir.path() + QDir::separator() + zipFileName, filePath))
    {
        qWarning() << "Unable to move new file into place, moving original back and bailing out";
        QFile::rename(filePath + ".tmp", filePath);
        return false;
    }
    if (QFile::exists(filePath))
    {
        QFile::remove(filePath + ".tmp");
        return true;
    }
    return false;
}

QString ZipHandler::getAudioExt()
{
    return m_audioExt;
}

QStringList ZipHandler::getSupportedAudioExts()
{
    return audioExtensions;
}

void ZipHandler::loadExtensions()
{
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
    audioExtensions.append(".flac");
}

void ZipHandler::initializeZipFile()
{
    mz_zip_reader_end(m_archive);
    qzFile->close();
    qzFile->setFileName(m_zipFile);
    qzFile->open(QFile::ReadOnly);
    int fileHandle = qzFile->handle();
    czFile = fdopen(fileHandle, "rb");
    mz_zip_archive_file_stat fStat;
    if (!mz_zip_reader_init_cfile(m_archive,czFile,qzFile->size(),0))
    {
        m_zipFileValid = false;
        qWarning() << "Error opening zip file";
        return;
    }
    m_zipFileValid = true;
    unsigned int files = mz_zip_reader_get_num_files(m_archive);
    for (unsigned int i=0; i < files; i++)
    {
        if (mz_zip_reader_file_stat(m_archive, i, &fStat))
        {
            QString fileName = fStat.m_filename;
            if (fileName == "ReplayGainProcessed")
            {
                m_rgMarkerFileFound = true;
            }
            else if (fileName.endsWith(".cdg",Qt::CaseInsensitive))
            {
                m_cdgFileName = fileName;
                m_cdgSize = fStat.m_uncomp_size;
                m_compressionSupportedCdg = fStat.m_is_supported;
                m_cdgFileFound = true;
            }
            else
            {
                for (int e=0; e < audioExtensions.size(); e++)
                {
                    if (fileName.endsWith(audioExtensions.at(e), Qt::CaseInsensitive))
                    {
                        m_audioFileName = fileName;
                        m_audioExt = audioExtensions.at(e);
                        m_audioSize = fStat.m_uncomp_size;
                        m_compressionSupportedAudio = fStat.m_is_supported;
                        m_audioFileFound = true;
                    }
                }
            }
        }
        else
        {
            m_zipFileValid = false;
            return;
        }
    }
}

ZipHandler::ZipHandler(QString zipFile, QObject *parent) :
    QObject(parent)
{
    settings = new Settings(this);
    m_archive = new mz_zip_archive;
    mz_zip_zero_struct(m_archive);
    qzFile = new QFile(this);
    loadExtensions();
    m_zipFile = zipFile;
    resetVars();
    initializeZipFile();
}

ZipHandler::ZipHandler(QObject *parent) :
    QObject(parent)
{
    settings = new Settings(this);
    m_archive = new mz_zip_archive;
    mz_zip_zero_struct(m_archive);
    qzFile = new QFile(this);
    loadExtensions();
    m_zipFile = "";
    resetVars();
}

bool ZipHandler::extractAudio(QDir destDir)
{
    if (m_audioFileFound && m_compressionSupportedAudio)
    {
        QString outFile(destDir.path() + QDir::separator() + "tmp" + m_audioExt);
        if (mz_zip_reader_extract_file_to_file(m_archive, m_audioFileName.toLocal8Bit().data(),outFile.toLocal8Bit().data(),0))
        {
            return true;
        }
    }
    return false;
}

bool ZipHandler::extractCdg(QDir destDir)
{
    if (m_cdgFileFound && m_compressionSupportedCdg)
    {
        QString outFile(destDir.path() + QDir::separator() + "tmp.cdg");
        if (mz_zip_reader_extract_file_to_file(m_archive, m_cdgFileName.toLocal8Bit().data(),outFile.toLocal8Bit().data(),0))
        {
            return true;
        }
    }
    return false;
}

QString ZipHandler::zipFile() const
{
    return m_zipFile;
}

void ZipHandler::setZipFile(const QString &zipFile)
{
    m_zipFile = zipFile;
    resetVars();
    initializeZipFile();
}

int ZipHandler::getSongDuration()
{
    return ((m_cdgSize / 96) / 75) * 1000;
}

bool ZipHandler::createZip(QString zipFile, QString cdgFile, QString audioFile, bool addRgMarker)
{
    QString audioFileName, cdgFileName;
    if (m_compressFilesWithZipFn)
    {
        QString zipFileBaseName, audioFileExt;
        zipFileBaseName = QFileInfo(zipFile).completeBaseName();
        audioFileExt = QFileInfo(audioFile).suffix();
        cdgFileName = zipFileBaseName + ".cdg";
        audioFileName = zipFileBaseName + "." + audioFileExt;
    }
    else
    {
        audioFileName = QFileInfo(audioFile).fileName();
        cdgFileName = QFileInfo(cdgFile).fileName();
    }
    qWarning() << "Creating zip file: " << zipFile;
    qWarning() << "Adding: " << cdgFile << " as " << cdgFileName;
    qWarning() << "Adding: " << audioFile << " as " << audioFileName;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_writer_init_file(&zip_archive, zipFile.toLocal8Bit().data(),0))
    {
        qWarning() << "Error initializing zip file";
    }
    const char *s_pComment = "File has been processed through ReplayGain with OpenKJ KaraokeRG";
    qWarning() << "ZipHandler::createZip(" << zipFile << ", " << cdgFile << ", " << audioFile << ")";
    qWarning() << "Adding mp3 file";
    if (!mz_zip_writer_add_file(&zip_archive, audioFileName.toLocal8Bit().data(), audioFile.toLocal8Bit().data(), NULL, 0, m_compressionLevel))
    {
        qWarning() << "Error adding mp3 file to archive";
        mz_zip_writer_finalize_archive(&zip_archive);
        mz_zip_writer_end(&zip_archive);
        return false;
    }
    qWarning() << "Adding cdg file";
    if (!mz_zip_writer_add_file(&zip_archive, cdgFileName.toLocal8Bit().data(), cdgFile.toLocal8Bit().data(), NULL, 0, m_compressionLevel))
    {
        qWarning() << "Error adding cdg file to archive";
        mz_zip_writer_finalize_archive(&zip_archive);
        mz_zip_writer_end(&zip_archive);
        return false;
    }
    if (addRgMarker)
    {
        qWarning() << "Adding marker to file";
        if (!mz_zip_writer_add_mem(&zip_archive,"ReplayGainProcessed", s_pComment, sizeof(s_pComment), m_compressionLevel))
        {
            qWarning() << "Error adding marker file to archive";
            mz_zip_writer_finalize_archive(&zip_archive);
            mz_zip_writer_end(&zip_archive);
            return false;
        }
    }
    qWarning() << "Files added to archive";
    mz_zip_writer_finalize_archive(&zip_archive);
    mz_zip_writer_end(&zip_archive);
    return true;
}

bool ZipHandler::isValidZipfile()
{
    return m_zipFileValid;
}

bool ZipHandler::containsRGMarkerFile()
{
    return m_rgMarkerFileFound;
}

bool ZipHandler::compressionSupported()
{
    if (m_compressionSupportedAudio && m_compressionSupportedCdg)
        return true;
    return false;
}

bool ZipHandler::containsAudioFile()
{
    return m_audioFileFound;
}

bool ZipHandler::containsCdgFile()
{
    return m_cdgFileFound;
}

void ZipHandler::setCompressionLevel(int level)
{
    m_compressionLevel = level;
}

void ZipHandler::setReplaceFnsWithZipFn(bool replace)
{
    m_compressFilesWithZipFn = replace;
}
