#include "dlgsettings.h"
#include "ui_dlgsettings.h"
#include <QFileDialog>

DlgSettings::DlgSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSettings)
{
    ui->setupUi(this);
    settings = new Settings(this);
    ui->lblMp3GainPath->setText(settings->mp3GainPath());
    ui->label7zipPath->setText(settings->get7zipPath());
    ui->labelFfmpegPath->setText(settings->ffmpegPath());
}

DlgSettings::~DlgSettings()
{
    delete ui;
}

void DlgSettings::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("mp3gain executable"), "", tr("All Files (*)"));
    if (fileName != "")
    {
        settings->setMp3GainPath(fileName);
        ui->lblMp3GainPath->setText(fileName);
    }
}

void DlgSettings::on_pushButton7zipBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("7zip executable"), "", tr("All Files (*)"));
    if (fileName != "")
    {
        settings->set7zipPath(fileName);
        ui->label7zipPath->setText(fileName);
    }
}

void DlgSettings::on_pushButtonFfmpegBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("ffmpeg executable"), "", tr("All Files (*)"));
    if (fileName != "")
    {
        settings->setFfmpegPath(fileName);
        ui->labelFfmpegPath->setText(fileName);
    }
}
