#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "processingthread.h"
#include "settings.h"
#include "dlgsettings.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnBrowse_clicked();
    void on_btnStart_clicked();
    void threadDone();
    void threadAborted();
    void fileProcessed();
    void on_actionSettings_activated();

    void on_checkBoxForce_toggled(bool checked);

private:
    void getFiles(QString fileExt = ".zip");
    Ui::MainWindow *ui;
    QString m_path;
    QStringList *m_files;
    ProcessingThread *m_thread1;
    ProcessingThread *m_thread2;
    ProcessingThread *m_thread3;
    ProcessingThread *m_thread4;
    int m_numProcessed;
    int m_threads;
    int m_threadsDone;
    int m_threadsAborted;
    Settings *settings;
    DlgSettings *dlgSettings;

};

#endif // MAINWINDOW_H
