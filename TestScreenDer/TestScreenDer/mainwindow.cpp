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

    areaSelector = new AreaSelector();
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
    connect(this                    , SIGNAL( signal_close() )    , areaSelector, SLOT( close() ) );
    connect(this                    , SIGNAL( signal_selection() ), areaSelector, SLOT( slot_init() ) );
    connect(ui->pushButtonSelectArea, SIGNAL( toggled(bool) )     , areaSelector, SLOT( setVisible( bool ) ) );

    connect(ui->pushButtonStart, SIGNAL(clicked(bool)), areaSelector, SLOT(slot_recordMode(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent( QCloseEvent *event )
{
    Q_UNUSED(event);
    emit signal_close();
}


void MainWindow::on_pushButtonSelectArea_clicked()
{
    bool state = ui->pushButtonSelectArea->isChecked();
    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    if(state){
        emit signal_selection();
    }
}

void MainWindow::on_pushButtonFullscreen_clicked()
{
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);
    areaSelector->setVisible(false);
}

void MainWindow::on_toolButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory", QDir::homePath());
    ui->lineEditPath->setText(path);
}
