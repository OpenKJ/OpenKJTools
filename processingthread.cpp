#include "processingthread.h"
#include "ziphandler.h"
#include <QTemporaryDir>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QMutex>

ProcessingThread::ProcessingThread(ProcessingType type, QObject *parent) :
    QThread(parent)
{
    m_terminate = false;
    settings = new Settings(this);
    m_force = settings->forceReprocessing();
    mp3gainPath = settings->mp3GainPath();
    processingType = type;
}

void ProcessingThread::setFiles(QStringList *files)
{
    m_files = files;
}

void ProcessingThread::processFileRg(QString fileName)
{
    qWarning() << "Processing - File: " << fileName;
    QFileInfo info(fileName);
    QString baseName = info.completeBaseName();
    emit stateChanged("Extracting file");
    QTemporaryDir tmpDir;
    ZipHandler zipper;
    zipper.setZipFile(fileName);   
    if ((zipper.containsRGMarkerFile()) && (!m_force))
    {
        qWarning() << "File contains marker, already processed.  Skipping";
        emit stateChanged("Skipping file, already processed");
        return;
    }
    qWarning() << "Processing - Extracting mp3 file";
    if (!zipper.extractAudio(QDir(tmpDir.path())))
    {
        qWarning() << "Bad file contents - error extracting mp3";
        emit stateChanged("Error extracting file");
        return;
    }
    QFile::copy(tmpDir.path() + QDir::separator() + "tmp.mp3", "/storage/KaraokeRGTest/PreRG.mp3");
    qWarning() << "Processing - Extracting cdg file";
    if (!zipper.extractCdg(QDir(tmpDir.path())))
    {
        qWarning() << "Processing - Bad file contents - error extracting cdg";
        emit stateChanged("Error extracting file");
        return;
    }
    emit stateChanged("Processing - Doing ReplayGain analasis and adjustment");
    QString program = mp3gainPath;
    QStringList arguments;
    arguments << "-c";
    arguments << "-r";
    arguments << "-T";
    arguments << "-s" << "r";
    arguments << tmpDir.path() + QDir::separator() + "tmp.mp3";
    QProcess process;
    process.start(program, arguments);
    process.waitForFinished();
    int exitCode = process.exitCode();
    QFile::copy(tmpDir.path() + QDir::separator() + "tmp.mp3", "/storage/KaraokeRGTest/PostRG.mp3");
    qWarning() << process.readAllStandardOutput();
    //qWarning() << process.readAllStandardError();
    if (exitCode != 0)
    {
        qWarning() << "Error occurred while running mp3gain, aborting";
        return;
    }
    qWarning() << "Processing - Creating zip file";
    emit stateChanged("Creating zip file");
    QFile::rename(tmpDir.path() + QDir::separator() + "tmp.mp3", tmpDir.path() + QDir::separator() + baseName + ".mp3");
    QFile::rename(tmpDir.path() + QDir::separator() + "tmp.cdg", tmpDir.path() + QDir::separator() + baseName + ".cdg");
    zipper.setReplaceFnsWithZipFn(false);
    zipper.setCompressionLevel(settings->zipCompressionLevel());
    if (zipper.createZip(tmpDir.path() + QDir::separator() + info.fileName(), tmpDir.path() + QDir::separator() + baseName + ".cdg", tmpDir.path() + QDir::separator() + baseName + ".mp3", true))
    {
        qWarning () << "Processing - Replacing original file";
        emit stateChanged("Replacing original file");
        if (QFile(tmpDir.path() + QDir::separator() + info.fileName()).exists())
        {
            qWarning() << "Doing QFile::rename(" << info.absoluteFilePath() << ", " << info.absoluteFilePath() + ".tmp" << ");";
            if (QFile::rename(info.absoluteFilePath(), info.absoluteFilePath() + ".tmp"))
            {
                if (QFile::copy(tmpDir.path() + QDir::separator() + info.fileName(), info.absoluteFilePath()))
                {
                    qWarning() << "New file copied into place, deleting old one";
                    if (!QFile::remove(info.absoluteFilePath() + ".tmp"))
                    {
                        qWarning() << "Error deleting old file";
                    }
                }
                else
                {
                    qWarning() << "Unable to move new file into place, restoring old one";
                    QFile::rename(info.absoluteFilePath() + ".tmp", info.absoluteFilePath());
                }

            }
            else
            {
                qWarning() << "Unable to move existing file to tmp file";
                return;
            }
        }
        else
        {
            qWarning() << "Unexpected error, new processed zip file missing!";
        }
        emit stateChanged("Idle");
        qWarning() << "Processing - Complete for file: " << fileName;
    }
    else
    {
        qWarning() << "Failed to create new archive, leaving original file in place";
    }
}

void ProcessingThread::processFileZipFix(QString fileName)
{
    qWarning() << "Processing - File: " << fileName;
    emit stateChanged("Checking file");
    ZipHandler zipper;
    zipper.setZipFile(fileName);
    if (!zipper.isValidZipfile())
    {
        qWarning() << "File doesn't appear to be a valid zip at all! Skipping";
        emit stateChanged("Skipping zipfile, appears to be an invalid or bad zip file!");
        return;
    }
    if (!zipper.containsAudioFile())
    {
        qWarning() << "Zipfile missing audio file. Skipping";
        emit stateChanged("Skipping zipfile, missing audio file.  May not be a karaoke file at all.");
        return;
    }
    if (!zipper.containsCdgFile())
    {
        qWarning() << "Zipfile missing cdg file. Skipping";
        emit stateChanged("Skipping file, missing audio file.  May not be a karaoke file at all.");
        return;
    }
    if (zipper.compressionSupported())
    {
        qWarning() << "File compression methods are supported.  Skipping";
        emit stateChanged("Skipping file, already using standard zip compression");
        return;
    }
    else
    {
        qWarning() << "File using unsupported compression method, converting.";
        emit stateChanged("Converting zip file");
        zipper.setReplaceFnsWithZipFn(true);
        zipper.setCompressionLevel(settings->zipCompressionLevel());
        zipper.reZipUnsupported(fileName);
    }
}

void ProcessingThread::processFileUnzip(QString fileName)
{
    qWarning() << "Processing - File: " << fileName;
    emit stateChanged("Checking file");
    ZipHandler zipper;
    zipper.setZipFile(fileName);
    if (!zipper.isValidZipfile())
    {
        qWarning() << "File doesn't appear to be a valid zip at all! Skipping";
        emit stateChanged("Skipping zipfile, appears to be an invalid or bad zip file!");
        return;
    }
    if (!zipper.containsAudioFile())
    {
        qWarning() << "Zipfile missing audio file. Skipping";
        emit stateChanged("Skipping zipfile, missing audio file.  May not be a karaoke file at all.");
        return;
    }
    if (!zipper.containsCdgFile())
    {
        qWarning() << "Zipfile missing cdg file. Skipping";
        emit stateChanged("Skipping file, missing audio file.  May not be a karaoke file at all.");
        return;
    }
    if (!zipper.compressionSupported())
    {
        qWarning() << "File compression method not supported.  Fixing.";
        emit stateChanged("File compression method not supported.  Fixing before continuing.");
        zipper.setReplaceFnsWithZipFn(true);
        zipper.setCompressionLevel(settings->zipCompressionLevel());
        if (!zipper.reZipUnsupported(fileName))
        {
            emit stateChanged("Unable to fix unsupported file.  Skipping.");
            qWarning() << "Unable to fix unsupported file.  Skipping";
            return;
        }
    }
    QTemporaryDir tmpDir;
    qWarning() << "File appears to be a valid audio+g zipfile, unzipping.";
    emit stateChanged("Extracting zip file");
    if (!zipper.extractAudio(tmpDir.path()))
    {
        qWarning() << "Error extracting audio file";
        return;
    }
    if (!zipper.extractCdg(tmpDir.path()))
    {
        qWarning() << "Error extracting cdg file";
        return;
    }

    QString zipFileBaseName = QFileInfo(fileName).completeBaseName();
    QString cdgFileName = zipFileBaseName + ".cdg";
    QString audioFileName = zipFileBaseName + zipper.getAudioExt();
    QString destCdg = QFileInfo(fileName).absolutePath() + QDir::separator() + cdgFileName;
    QString destAud = QFileInfo(fileName).absolutePath() + QDir::separator() + audioFileName;
    if (QFile::exists(destCdg) || QFile::exists(destAud))
    {
        qWarning() << "Found existing cdg or mp3 file in the destination dir, bailing out";
        return;
    }
    if (!QFile::copy(tmpDir.path() + QDir::separator() + "tmp.cdg", destCdg))
    {
        qWarning() << "Error copying cdg file to destination dir, bailing out";
        return;
    }
    if (!QFile::copy(tmpDir.path() + QDir::separator() + "tmp" + zipper.getAudioExt(), destAud))
    {
        qWarning() << "Error copying audio file to the destination dir, bailing out";
        return;
    }
    if (!QFile::exists(destCdg) || !QFile::exists(destAud))
    {
        qWarning() << "Destination cdg or audio file doesn't exist, something went wrong!  Bailing out";
        return;
    }
    if (settings->removeAfterUnzip())
    {
        if (!QFile::remove(fileName))
        {
            qWarning() << "Unable to remove original zip file. Bailing out.";
            return;
        }
        else
        {
            qWarning() << "Process completed successfully!";
        }
    }
}

void ProcessingThread::processFileZip(QString fileName)
{
    ZipHandler zipper;
    emit stateChanged("Searching for matching audio file");
    if (!QFile::exists(fileName))
    {
        qWarning() << "CDG file doesn't exist, bailing out";
        return;
    }
    QString audioExt;
    QStringList audioExts = zipper.getSupportedAudioExts();
    QString path = QFileInfo(fileName).absolutePath();
    QString baseName = QFileInfo(fileName).completeBaseName();
    QString audioFile;
    QString zipFile = path + QDir::separator() + baseName + ".zip";
    QString zipBaseName = baseName + ".zip";
    for (int i=0; i < audioExts.size(); i++)
    {
        if (QFile::exists(path + QDir::separator() + baseName + audioExts.at(i)))
        {
            qWarning() << "Audio file found, continuing.";
            audioFile = path + QDir::separator() + baseName + audioExts.at(i);
            break;
        }
    }
    if (audioFile == "")
    {
        qWarning() << "Matching audio file not found, bailing out";
        return;
    }
    emit stateChanged("Creating zip file");
    if (QFile::exists(zipFile))
    {
        qWarning() << "Zip file already exists, bailing out";
        return;
    }
    QTemporaryDir tmpDir;
    if (!zipper.createZip(tmpDir.path() + QDir::separator() + zipBaseName, fileName, audioFile))
    {
        qWarning() << "Error creating temporary zip file, bailing out";
        return;
    }
    if (!QFile::exists(tmpDir.path() + QDir::separator() + zipBaseName))
    {
        qWarning() << "Temporary zip file doesn't exist, bailing out";
        return;
    }
    emit stateChanged("Copying zip file to final location");
    if (!QFile::copy(tmpDir.path() + QDir::separator() + zipBaseName, zipFile))
    {
        qWarning() << "Error copying zip file to final destination, bailing out";
        return;
    }
    if (settings->removeAfterZip())
    {
        emit stateChanged("Removing original cdg and audio file");
        if (!QFile::remove(fileName))
        {
            qWarning() << "Error removing cdg file, bailing out";
            return;
        }
        if (!QFile::remove(audioFile))
        {
            qWarning() << "Error removing audio file, bailing out";
            return;
        }
    }
    emit stateChanged("Finished processing");
    qWarning() << "Zipfile created successfully";

}


QMutex mutex;
void ProcessingThread::run()
{
    while (!m_terminate)
    {
        mutex.lock();
        if (m_files->size() > 0)
        {
            m_currentFile = m_files->at(0);
            m_files->removeFirst();
            mutex.unlock();
        }
        else
        {
            mutex.unlock();
            emit processingComplete();
            emit processingFile("N/A");
            return;
        }
        emit processingFile(m_currentFile);
        qWarning() << "Processing type set to: " << processingType;
        if (processingType == ProcessingType::REPLAY_GAIN)
            processFileRg(m_currentFile);
        else if (processingType == ProcessingType::ZIPFIX)
            processFileZipFix(m_currentFile);
        else if (processingType == ProcessingType::UNZIP)
            processFileUnzip(m_currentFile);
        else if (processingType == ProcessingType::ZIP)
            processFileZip(m_currentFile);
        emit fileProcessed();

    }
    emit processingFile("N/A");
    emit processingAborted();
}

void ProcessingThread::stopProcessing()
{
    m_terminate = true;
}
