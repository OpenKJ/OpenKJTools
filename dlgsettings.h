#ifndef DLGSETTINGS_H
#define DLGSETTINGS_H

#include <QDialog>
#include "settings.h"

namespace Ui {
class DlgSettings;
}

class DlgSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSettings(QWidget *parent = 0);
    ~DlgSettings();

private slots:
    void on_btnBrowse_clicked();

    void on_pushButton7zipBrowse_clicked();

    void on_pushButtonFfmpegBrowse_clicked();

private:
    Ui::DlgSettings *ui;
    Settings *settings;
};

#endif // DLGSETTINGS_H
