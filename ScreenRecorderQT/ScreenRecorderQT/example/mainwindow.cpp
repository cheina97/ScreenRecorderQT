#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    areaSelector = new AreaSelector();

    // connect
     connect(this, SIGNAL(signal_close()), areaSelector, SLOT(close()));
     connect(this, SIGNAL(signal_show(bool)), areaSelector,
             SLOT(setVisible(bool)));
     connect(this, SIGNAL(signal_recording(bool)), areaSelector,
             SLOT(slot_recordMode(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    static bool first_call = true;
    if (first_call) {
        first_call = false;
        areaSelector->slot_init();
    }
    emit signal_show(true);
    qDebug() << "aaaaaaa";
}


void MainWindow::on_pushButton_2_clicked()
{
    emit signal_show(false);
}


void MainWindow::on_pushButton_3_clicked()
{
    emit signal_recording(true);
}


void MainWindow::on_pushButton_4_clicked()
{
    emit signal_recording(false);
}

