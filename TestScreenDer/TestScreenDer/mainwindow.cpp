#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //button properties
    ui->pushButtonFullscreen->setCheckable(true);
    ui->pushButtonFullscreen->setIcon(QIcon("../Icons/fullscreen.png"));

    ui->pushButtonSelectArea->setCheckable(true);
    ui->pushButtonSelectArea->setIcon(QIcon("../Icons/area.png"));

    ui->pushButtonPause->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);

    //line edit default text
    ui->lineEditPath->setText(QDir::homePath());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonSelectArea_clicked()
{
    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    qDebug() << "Clicked";
    // TODO: qui si deve inserire il codice per la selezione dell'area.

}

void MainWindow::on_pushButtonFullscreen_clicked()
{
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);
    // TODO: settare un flag che dice "fullscreen selezionato"
}

void MainWindow::on_toolButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory", QDir::homePath());
    ui->lineEditPath->setText(path);
}
