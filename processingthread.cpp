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
    if (fileName.endsWith("mp4", Qt::CaseInsensitive))
    {
        qWarning() << "Processing - mp4 file: " << fileName;
        QString program = mp3gainPath;
        QStringList arguments;
        arguments << "-c";
        arguments << "-r";
        arguments << "-T";
        arguments << "-s" << "r";
        arguments << fileName;
        emit stateChanged("Processing - Doing ReplayGain analasis and adjustment");
        QProcess process;
        process.start(program, arguments);
        process.waitForFinished();
        int exitCode = process.exitCode();
        if (exitCode != 0)
        {
            qWarning() << "Error occurred while running aacgain, aborting";
            return;
        }
        emit stateChanged("Idle");
        return;
    }
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
        zipper.close();
        return;
    }
    qWarning() << "Processing - Extracting mp3 file";
    if (!zipper.extractAudio(QDir(tmpDir.path())))
    {
        qWarning() << "Bad file contents - error extracting mp3";
        emit stateChanged("Error extracting file");
        zipper.close();
        return;
    }
    QFile::copy(tmpDir.path() + QDir::separator() + "tmp.mp3", "/storage/KaraokeRGTest/PreRG.mp3");
    qWarning() << "Processing - Extracting cdg file";
    if (!zipper.extractCdg(QDir(tmpDir.path())))
    {
        qWarning() << "Processing - Bad file contents - error extracting cdg";
        emit stateChanged("Error extracting file");
        zipper.close();
        return;
    }
    emit stateChanged("Processing - Doing ReplayGain analasis and adjustment");
    zipper.close();
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
        zipper.setZipFile(fileName);
    }
    if (!zipper.isValidZipfile())
    {
        qWarning() << "File doesn't appear to be a valid zip at all! Skipping";
        emit stateChanged("Skipping zipfile, appears to be an invalid or bad zip file!");
        zipper.close();
        return;
    }
    if (!zipper.containsAudioFile())
    {
        qWarning() << "Zipfile missing audio file. Skipping";
        emit stateChanged("Skipping zipfile, missing audio file.  May not be a karaoke file at all.");
        zipper.close();
        return;
    }
    if (!zipper.containsCdgFile())
    {
        qWarning() << "Zipfile missing cdg file. Skipping";
        emit stateChanged("Skipping file, missing audio file.  May not be a karaoke file at all.");
        zipper.close();
        return;
    }

    QTemporaryDir tmpDir;
    qWarning() << "File appears to be a valid audio+g zipfile, unzipping.";
    emit stateChanged("Extracting zip file");
    if (!zipper.extractAudio(tmpDir.path()))
    {
        qWarning() << "Error extracting audio file";
        zipper.close();
        return;
    }
    if (!zipper.extractCdg(tmpDir.path()))
    {
        qWarning() << "Error extracting cdg file";
        zipper.close();
        return;
    }
    zipper.close();
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
            // May be a lagged out file lock on remote samba shares, sleep a bit and try again...
            emit stateChanged("Deleting original file failed, sleeping a bit and trying again in case of stale file locks...");
            this->msleep(1000);
            if (!QFile::remove(fileName))
            {
                qWarning() << "Unable to remove original zip file. Bailing out.";
                emit stateChanged("Delete failed!");
                return;
            }
            else
            {
                qWarning() << "Process completed successfully!";
                emit stateChanged("Done!");
            }
        }
        else
        {
            qWarning() << "Process completed successfully!";
            emit stateChanged("Done!");
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

void ProcessingThread::processFileCaseFix(QString fileName)
{
    emit stateChanged("Fixing case");
    QString sep = " - ";
    int sidSection = 0;
    QFileInfo fileinfo(fileName);
    QString iFN = fileinfo.completeBaseName();
    QStringList nocaps;
    nocaps << "a";
    nocaps << "an";
    nocaps << "the";
    nocaps << "at";
    nocaps << "by";
    nocaps << "for";
    nocaps << "in";
    nocaps << "of";
    nocaps << "on";
    nocaps << "to";
    nocaps << "up";
    nocaps << "and";
    nocaps << "as";
    nocaps << "but";
    nocaps << "or";
    nocaps << "nor";

    QStringList capsafter;
    capsafter << "Mc";
    capsafter << "O'";
    capsafter << "(";
//    capsafter << "-";
//    capsafter << ".";

    QStringList exceptions;
    exceptions << "ACDC";
    exceptions << "AC-DC";
    exceptions << "Rag'n'Bone";
    exceptions << "3LW";
    exceptions << "XTC";
    exceptions << "MC";
    exceptions << "DJ";
    exceptions << "SZA";
    exceptions << "YOLO";
    exceptions << "24K";
    exceptions << "...Ready";
    exceptions << "FourFiveSeconds";
    exceptions << "II";
    exceptions << "II)";
    exceptions << "ZZ";
    exceptions << "'N";
    exceptions << "KC";
    exceptions << "PJ";
    exceptions << "'Til";
    exceptions << "NIN";
    exceptions << "OMC";
   // exceptions << "LA";
    exceptions << "UB40";

    //exceptions << "s";

    // fix common naming problems before processing
    QString pfFN = iFN;
    pfFN.replace(" let s ", " let's ", Qt::CaseInsensitive);
    pfFN.replace(" I M ",  " I'm ", Qt::CaseInsensitive);
    pfFN.replace(" that s ", " that's ", Qt::CaseInsensitive);
    pfFN.replace(" don t ", " don't ", Qt::CaseInsensitive);
    pfFN.replace(" won t ", " won't ", Qt::CaseInsensitive);
    pfFN.replace(" here s ", " here's ", Qt::CaseInsensitive);
    pfFN.replace(" ex s ", " ex's ", Qt::CaseInsensitive);
    pfFN.replace(" oh s ", " oh's ", Qt::CaseInsensitive);
    pfFN.replace(" can t ", " can't ", Qt::CaseInsensitive);
    pfFN.replace(" it s ", " it's ", Qt::CaseInsensitive);
    pfFN.replace(" hadn t ", " hadn't ", Qt::CaseInsensitive);
    pfFN.replace(" we re ", " we're ", Qt::CaseInsensitive);
    pfFN.replace(" donesn t ", " donesn't ", Qt::CaseInsensitive);
    pfFN.replace(" you s ", " you's ", Qt::CaseInsensitive);





    QStringList sections = pfFN.split(" - ");
    QString oFN;

    for (int s=0; s < sections.size(); s++)
    {

        QStringList parts = sections.at(s).split(" ", QString::SkipEmptyParts);
        QString oPart;
        for (int i=0; i < parts.size(); i++)
        {
            if (s == sidSection)
            {
                if (s != 0)
                    oFN += " - ";
                oFN += parts.at(i).toUpper();
                continue;
            }
            QString part = parts.at(i);
            bool match = false;
            for (int e=0; e < exceptions.size(); e++)
            {
                if (exceptions.at(e).toLower() == part.toLower())
                {
                    if (i != 0)
                        oPart += " ";
                    oPart += exceptions.at(e);
                    match = true;
                    break;
                }
            }
            if (match)
                continue;
            part = part.toLower();
            if ((nocaps.contains(part)) && (i != 0) && (i != parts.size() - 1))
            {
                if (!parts.at(i + 1).startsWith("(") && !parts.at(i + 1).startsWith("["))
                {
                    if (i != 0)
                        oPart += " ";
                    oPart += part;
                    continue;
                }
            }
            part.replace(0,1,part.at(0).toUpper());
            for (int j=0; j < capsafter.size(); j++)
            {
                if ((part.startsWith(capsafter.at(j))) && (part.size() > capsafter.at(j).size()))
                {
                    part.replace(capsafter.at(j).size(), 1, part.at(capsafter.at(j).size()).toUpper());
                }

            }
            for (int z=0; z < part.size(); z++)
            {
                if (z < part.size() - 1)
                {
                    if ((part.at(z) == QString(".")) || (part.at(z) == QString("-")))
                    {
                        QString curLetter = part.at(z + 1);
                        part.replace(z + 1, 1, curLetter.toUpper());
                    }
                }
            }
            if (i != 0)
                oPart += " ";
            oPart += part;
        }
        if (s != 0)
            oFN += " - ";
        oFN += oPart;
    }
    if (iFN != oFN)
    {
        emit stateChanged("Renaming file");
        qWarning() << "iFN: " << iFN << "\noFN: " << oFN << "\nRenaming to: " << QString(fileinfo.absolutePath() + QDir::separator() + oFN + "." + fileinfo.suffix());
        QFile::rename(fileName, fileName + ".rntmp");
        QFile::rename(fileName + ".rntmp", fileinfo.absolutePath() + QDir::separator() + oFN + "." + fileinfo.suffix());
    }
}

void ProcessingThread::processFileCdg2Mp4(QString fileName)
{
    if (fileName.endsWith(".cdg", Qt::CaseInsensitive))
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
        QString mp4file = path + QDir::separator() + baseName + ".mp4";
        convertMp3g2Mp4(fileName, audioFile, mp4file);
    }
    else if (fileName.endsWith(".zip", Qt::CaseInsensitive))
    {





        qWarning() << this->objectName() << " - Processing - File: " << fileName;
        emit stateChanged("Checking file");
        ZipHandler zipper;
        zipper.setZipFile(fileName);
        if (!zipper.compressionSupported())
        {
            qWarning() << this->objectName() << " - File compression method not supported.  Fixing.";
            emit stateChanged("File compression method not supported.  Fixing before continuing.");
            zipper.setReplaceFnsWithZipFn(true);
            zipper.setCompressionLevel(settings->zipCompressionLevel());
            if (!zipper.reZipUnsupported(fileName))
            {
                emit stateChanged("Unable to fix unsupported file.  Skipping.");
                qWarning() << this->objectName() << " - Unable to fix unsupported file.  Skipping";
                return;
            }
            zipper.setZipFile(fileName);
        }
        if (!zipper.isValidZipfile())
        {
            qWarning() << this->objectName() << " - File doesn't appear to be a valid zip at all! Skipping";
            emit stateChanged("Skipping zipfile, appears to be an invalid or bad zip file!");
            zipper.close();
            return;
        }
        if (!zipper.containsAudioFile())
        {
            qWarning() << this->objectName() << " - Zipfile missing audio file. Skipping";
            emit stateChanged("Skipping zipfile, missing audio file.  May not be a karaoke file at all.");
            zipper.close();
            return;
        }
        if (!zipper.containsCdgFile())
        {
            qWarning() << this->objectName() << " - Zipfile missing cdg file. Skipping";
            emit stateChanged("Skipping file, missing audio file.  May not be a karaoke file at all.");
            zipper.close();
            return;
        }

        QTemporaryDir tmpDir;
        qWarning() << this->objectName() << " - File appears to be a valid audio+g zipfile, unzipping.";
        emit stateChanged("Extracting zip file");
        if (!zipper.extractAudio(tmpDir.path()))
        {
            qWarning() << this->objectName() << " - Error extracting audio file";
            zipper.close();
            return;
        }
        if (!zipper.extractCdg(tmpDir.path()))
        {
            qWarning() << this->objectName() << " - Error extracting cdg file";
            zipper.close();
            return;
        }
        zipper.close();
        QString zipFileBaseName = QFileInfo(fileName).completeBaseName();
        QString cdgFileName = zipFileBaseName + ".cdg";
        QString audioFileName = zipFileBaseName + zipper.getAudioExt();
        QString destCdg = tmpDir.path() + QDir::separator() + cdgFileName;
        QString destAud = tmpDir.path() + QDir::separator() + audioFileName;
        if (QFile::exists(destCdg) || QFile::exists(destAud))
        {
            qWarning() << this->objectName() << " - Found existing cdg or mp3 file in the destination dir, bailing out";
            return;
        }
        if (!QFile::copy(tmpDir.path() + QDir::separator() + "tmp.cdg", destCdg))
        {
            qWarning() << this->objectName() << " - Error copying cdg file to destination dir, bailing out";
            return;
        }
        if (!QFile::copy(tmpDir.path() + QDir::separator() + "tmp" + zipper.getAudioExt(), destAud))
        {
            qWarning() << this->objectName() << " - Error copying audio file to the destination dir, bailing out";
            return;
        }
        if (!QFile::exists(destCdg) || !QFile::exists(destAud))
        {
            qWarning() << this->objectName() << " - Destination cdg or audio file doesn't exist, something went wrong!  Bailing out";
            return;
        }

                QString path = QFileInfo(fileName).absolutePath();
                QString baseName = QFileInfo(fileName).completeBaseName();

                QString mp4file = path + QDir::separator() + baseName + ".mp4";
                convertMp3g2Mp4(destCdg, destAud, mp4file);
    }
}

bool ProcessingThread::convertMp3g2Mp4(QString cdgFile, QString mp3File, QString mp4file)
{
    emit stateChanged("Converting to mp4...");
    qWarning() << this->objectName() << " - Converting " << cdgFile << " and " << mp3File << " to " << mp4file;

    QProcess *process = new QProcess(this);
    QStringList arguments;
    arguments << "-n";
    arguments << "-itsoffset";
    arguments << "0.5";
    arguments << "-i";
    arguments << cdgFile;
    arguments << "-i";
    arguments << mp3File;
    arguments << "-s";
    arguments << "300x216";
    arguments << "-c:a";
    arguments << "copy";
    arguments << "-r";
    arguments << "24";
    arguments << "-c:v";
    arguments << "libx264";
    arguments << "-crf";
    arguments << "28";
    arguments << "-preset";
    arguments << "fast";
    arguments << "-tune";
    arguments << "animation";
    arguments << "-map";
    arguments << "0:0";
    arguments << "-map";
    arguments << "1:0";
    arguments << mp4file;

    process->start(settings->ffmpegPath(), arguments);
    process->waitForFinished();
    qWarning() << process->readAllStandardOutput();
    qWarning() << process->readAllStandardError();
    if (process->exitCode() != 0)
    {
        emit stateChanged("Conversion FAILED!");
        qWarning() << this->objectName() << " - Conversion FAILED!";
        return false;
    }
    else
    {
        qWarning() << this->objectName() << " - Conversion completed successfully";
        emit stateChanged("Conversion complete");
        return true;
    }

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
        if (processingType == ProcessingThread::REPLAY_GAIN)
            processFileRg(m_currentFile);
        else if (processingType == ProcessingThread::ZIPFIX)
            processFileZipFix(m_currentFile);
        else if (processingType == ProcessingThread::UNZIP)
            processFileUnzip(m_currentFile);
        else if (processingType == ProcessingThread::ZIP)
            processFileZip(m_currentFile);
        else if (processingType == ProcessingThread::CASEFIX)
            processFileCaseFix(m_currentFile);
        else if (processingType == ProcessingThread::CDG2MP4)
            processFileCdg2Mp4(m_currentFile);
        emit fileProcessed();

    }
    emit processingFile("N/A");
    emit processingAborted();
}

void ProcessingThread::stopProcessing()
{
    m_terminate = true;
}
