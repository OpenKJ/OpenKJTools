#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_numProcessed = 0;
    m_threadsDone = 0;
    m_threadsAborted = 0;
    ui->setupUi(this);
    QApplication::setApplicationName("OpenKJ Tools");
    QApplication::setOrganizationName("OpenKJ");
    QApplication::setOrganizationDomain("OpenKJ.org");
    settings = new Settings(this);
    dlgSettings = new DlgSettings(this);
    m_threads = QThread::idealThreadCount();
    if (m_threads == 1)
    {
        ui->frameThread2->hide();
        ui->frameThread3->hide();
        ui->frameThread4->hide();
    }
    else if (m_threads == 2)
    {
        ui->frameThread3->hide();
        ui->frameThread4->hide();
    }
    else if (m_threads == 3)
    {
        ui->frameThread4->hide();
    }
    m_files = new QStringList;
    connect(ui->actionSettings, SIGNAL(triggered(bool)), this, SLOT(on_actionSettings_activated()));
    ui->checkBoxForce->setChecked(settings->forceReprocessing());

    ui->spinBoxZipLvl->setValue(settings->zipCompressionLevel());
    ui->spinBoxZipLvl2->setValue(settings->zipCompressionLevel());
    ui->spinBoxZipLvl3->setValue(settings->zipCompressionLevel());

    ui->checkBoxRemoveAfterUnZip->setChecked(settings->removeAfterUnzip());
    ui->checkBoxRemoveAfterZip->setChecked(settings->removeAfterZip());

    connect(ui->checkBoxRemoveAfterUnZip, SIGNAL(clicked(bool)), settings, SLOT(setRemoveAfterUnzip(bool)));
    connect(ui->checkBoxRemoveAfterZip, SIGNAL(clicked(bool)), settings, SLOT(setRemoveAfterZip(bool)));

    connect(settings, SIGNAL(zipCompressionLevelChanged(int)), ui->spinBoxZipLvl, SLOT(setValue(int)));
    connect(settings, SIGNAL(zipCompressionLevelChanged(int)), ui->spinBoxZipLvl2, SLOT(setValue(int)));
    connect(settings, SIGNAL(zipCompressionLevelChanged(int)), ui->spinBoxZipLvl3, SLOT(setValue(int)));

    connect(ui->spinBoxZipLvl, SIGNAL(valueChanged(int)), settings, SLOT(setZipCompressionLevel(int)));
    connect(ui->spinBoxZipLvl2, SIGNAL(valueChanged(int)), settings, SLOT(setZipCompressionLevel(int)));
    connect(ui->spinBoxZipLvl3, SIGNAL(valueChanged(int)), settings, SLOT(setZipCompressionLevel(int)));

    m_path = settings->lastPath();
    ui->labelPath->setText((settings->lastPath() != "") ? settings->lastPath() : "None");
    ui->btnStart->setEnabled((settings->lastPath() != ""));


}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_files;
}

void MainWindow::on_btnBrowse_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "Select directory to process", settings->lastPath());
    if (dirPath != "")
    {
        m_path = dirPath;
        ui->labelPath->setText(dirPath);
        ui->btnStart->setEnabled(true);
        QApplication::processEvents();
        settings->setLastPath(dirPath);
    }
}

void MainWindow::getFiles(QString fileExt)
{
    ui->labelStatus1_2->setText("Scanning for files");
    m_files->clear();
    QDir dir(m_path);
    QDirIterator iterator(dir.absolutePath(), QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        if (!iterator.fileInfo().isDir()) {
            QString filename = iterator.filePath();
            if (filename.endsWith(fileExt,Qt::CaseInsensitive))
                m_files->append(filename);
        }
        QApplication::processEvents();
    }
    qWarning() << "Files found: " << m_files->size();
    ui->progressBar->setMaximum(m_files->size());
    ui->progressBar->setValue(0);
    ui->labelStatus1_2->setText("Idle");
}

void MainWindow::on_btnStart_clicked()
{
    QString fileExt = ".zip";
    ProcessingThread::ProcessingType processingType;
    if (ui->tabWidget->currentIndex() == 0)
    {
        processingType = ProcessingThread::REPLAY_GAIN;
        ui->labelAction->setText("Current Action: KaroakeRG");
    }
    if (ui->tabWidget->currentIndex() == 3)
    {
        processingType = ProcessingThread::ZIPFIX;
        ui->labelAction->setText("Current Action: ZipFix");
    }
    if (ui->tabWidget->currentIndex() == 2)
    {
        processingType = ProcessingThread::UNZIP;
        ui->labelAction->setText("Current Action: Unzip");
    }
    if (ui->tabWidget->currentIndex() == 4)
    {
        processingType = ProcessingThread::CASEFIX;
        ui->labelAction->setText("Current Action: CaseFix");
    }
    if (ui->tabWidget->currentIndex() == 1)
    {
        fileExt = ".cdg";
        processingType = ProcessingThread::ZIP;
        ui->labelAction->setText("Current Action: Zip");
    }
    if (!QFile::exists(settings->mp3GainPath()))
    {
        QMessageBox msgBox;
        msgBox.setText("mp3gain not found!  Please set the mp3gain path in settings.");
        msgBox.setInformativeText("If you don't already have it installed, please install it on your sysetm.  It is required for the ReplayGain functionality");
        msgBox.exec();
        return;
    }
    if (!QFile::exists(settings->get7zipPath()))
    {
        QMessageBox msgBox;
        msgBox.setText("7zip not found!  Please set the 7zip path in settings.");
        msgBox.setInformativeText("If you don't already have it installed, please install it on your system.  It is required for the Zip Fix functionality");
        msgBox.exec();
        return;
    }
    getFiles(fileExt);
    m_numProcessed = 0;
    if (m_files->size() == 0)
        return;
    m_thread1 = new ProcessingThread(processingType, this);
    m_thread2 = new ProcessingThread(processingType, this);
    m_thread3 = new ProcessingThread(processingType, this);
    m_thread4 = new ProcessingThread(processingType, this);

    connect(m_thread1, SIGNAL(processingFile(QString)), ui->labelCurrentFile1_2, SLOT(setText(QString)));
    connect(m_thread2, SIGNAL(processingFile(QString)), ui->labelCurrentFile2_2, SLOT(setText(QString)));
    connect(m_thread3, SIGNAL(processingFile(QString)), ui->labelCurrentFile3_2, SLOT(setText(QString)));
    connect(m_thread4, SIGNAL(processingFile(QString)), ui->labelCurrentFile4_2, SLOT(setText(QString)));

    connect(m_thread1, SIGNAL(stateChanged(QString)), ui->labelStatus1_2, SLOT(setText(QString)));
    connect(m_thread2, SIGNAL(stateChanged(QString)), ui->labelStatus2_2, SLOT(setText(QString)));
    connect(m_thread3, SIGNAL(stateChanged(QString)), ui->labelStatus3_2, SLOT(setText(QString)));
    connect(m_thread4, SIGNAL(stateChanged(QString)), ui->labelStatus4_2, SLOT(setText(QString)));

    connect(m_thread1, SIGNAL(processingComplete()), this, SLOT(threadDone()));
    connect(m_thread2, SIGNAL(processingComplete()), this, SLOT(threadDone()));
    connect(m_thread3, SIGNAL(processingComplete()), this, SLOT(threadDone()));
    connect(m_thread4, SIGNAL(processingComplete()), this, SLOT(threadDone()));

    connect(m_thread1, SIGNAL(processingAborted()), this, SLOT(threadAborted()));
    connect(m_thread2, SIGNAL(processingAborted()), this, SLOT(threadAborted()));
    connect(m_thread3, SIGNAL(processingAborted()), this, SLOT(threadAborted()));
    connect(m_thread4, SIGNAL(processingAborted()), this, SLOT(threadAborted()));

    connect(m_thread1, SIGNAL(fileProcessed()), this, SLOT(fileProcessed()));
    connect(m_thread2, SIGNAL(fileProcessed()), this, SLOT(fileProcessed()));
    connect(m_thread3, SIGNAL(fileProcessed()), this, SLOT(fileProcessed()));
    connect(m_thread4, SIGNAL(fileProcessed()), this, SLOT(fileProcessed()));

    connect(ui->btnStop, SIGNAL(clicked()), m_thread1, SLOT(stopProcessing()));
    connect(ui->btnStop, SIGNAL(clicked()), m_thread2, SLOT(stopProcessing()));
    connect(ui->btnStop, SIGNAL(clicked()), m_thread3, SLOT(stopProcessing()));
    connect(ui->btnStop, SIGNAL(clicked()), m_thread4, SLOT(stopProcessing()));

    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    ui->btnBrowse->setEnabled(false);

    m_thread1->setFiles(m_files);
    m_thread1->start();

    if (m_threads >= 2)
    {
        m_thread2->setFiles(m_files);
        m_thread2->start();
    }
    if (m_threads >= 3)
    {
        m_thread3->setFiles(m_files);
        m_thread3->start();
    }
    if (m_threads >=4)
    {
        m_thread4->setFiles(m_files);
        m_thread4->start();
    }

}

void MainWindow::threadDone()
{
    m_threadsDone++;
    int total = m_threadsDone + m_threadsAborted;
    if ((total >= m_threads) || (total >= m_files->size()))
    {
        ui->btnStop->setEnabled(false);
        ui->btnBrowse->setEnabled(true);
        ui->btnStart->setEnabled(true);
        ui->labelCurrentFile1_2->setText("Idle");
        ui->labelCurrentFile2_2->setText("Idle");
        ui->labelCurrentFile3_2->setText("Idle");
        ui->labelCurrentFile4_2->setText("Idle");
        ui->labelStatus1_2->setText("N/A");
        ui->labelStatus2_2->setText("N/A");
        ui->labelStatus3_2->setText("N/A");
        ui->labelStatus4_2->setText("N/A");
        ui->labelAction->setText("Current Action: None");

    }
}

void MainWindow::threadAborted()
{
    m_threadsAborted++;
    int total = m_threadsDone + m_threadsAborted;
    if ((total >= m_threads) || (total >= m_files->size()))
    {
        ui->btnStop->setEnabled(false);
        ui->btnBrowse->setEnabled(true);
        ui->btnStart->setEnabled(true);
        ui->labelCurrentFile1_2->setText("Aborted");
        ui->labelCurrentFile2_2->setText("Aborted");
        ui->labelCurrentFile3_2->setText("Aborted");
        ui->labelCurrentFile4_2->setText("Aborted");
        ui->labelStatus1_2->setText("N/A");
        ui->labelStatus2_2->setText("N/A");
        ui->labelStatus3_2->setText("N/A");
        ui->labelStatus4_2->setText("N/A");
    }
}

void MainWindow::fileProcessed()
{
   // qWarning() << "MainWindow received signal fileProcessed()";
    m_numProcessed++;
    ui->progressBar->setValue(m_numProcessed);
}

void MainWindow::on_actionSettings_activated()
{
    dlgSettings->show();
}

void MainWindow::on_checkBoxForce_toggled(bool checked)
{
    settings->setForceReprocessing(checked);
}
