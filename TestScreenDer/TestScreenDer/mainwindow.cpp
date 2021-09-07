#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "AreaSelector.h"

#include <QDebug>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    areaSelector = new AreaSelector(ui);
    //TODO:
    //setWindowTitle( global::name + " " + global::version );
    //QIcon icon( QString::fromUtf8( ":/pictures/logo/logo.png" ) );
    //setWindowIcon( icon );

    //tab widget
    ui->tabWidget->setCurrentIndex(0); //always open on first tab

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

    //checkboxes
    ui->checkBoxStereo->setChecked(true);

    //radio buttons
    ui->radioButton24->setChecked(true);
    ui->radioButton60->setMouseTracking(true);
    ui->radioButton60->setToolTip("High performances required");

    //connect
    connect(ui->pushButtonSelectArea, SIGNAL(clicked()), areaSelector, SLOT(slot_init()));
    connect(ui->pushButtonSelectArea, SIGNAL( clicked(bool) ), areaSelector, SLOT( setVisible( bool ) ) );

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonSelectArea_clicked()
{
    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    // TODO: qui si deve inserire il codice per la selezione dell'area.

}

void MainWindow::on_pushButtonFullscreen_clicked()
{
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);
}

void MainWindow::on_toolButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory", QDir::homePath());
    ui->lineEditPath->setText(path);
}
