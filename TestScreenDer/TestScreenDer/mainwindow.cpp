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

    setWindowTitle( "ScreenCapture");
    QIcon icon( QString::fromUtf8( ":/icons/mainIcon.jpg" ) );
    setWindowIcon( icon );

    //tab widget
    ui->tabWidget->setCurrentIndex(0); //always open on first tab

    //button properties
    ui->pushButtonFullscreen->setCheckable(true);
    ui->pushButtonFullscreen->setIcon(QIcon(":/icons/fullscreen.png"));

    ui->pushButtonSelectArea->setCheckable(true);
    ui->pushButtonSelectArea->setIcon(QIcon(":/icons/area.png"));

    ui->pushButtonPause->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);

    //line edit default text
    ui->lineEditPath->setText(QDir::homePath());

    //audio default selection
    ui->radioButtonYes->setChecked(true);

    //radio buttons
    ui->radioButton24->setChecked(true);
    ui->radioButton60->setMouseTracking(true);
    ui->radioButton60->setToolTip("High performances required");

    //checkbox properties
    ui->checkBoxMinimize->setChecked(true);

    //slider properties
    ui->horizontalSlider->setTracking( true );
    ui->horizontalSlider->setMinimum( 1 );
    ui->horizontalSlider->setMaximum( 3 );
    ui->horizontalSlider->setValue( 2 );

    //connect
    connect(this , SIGNAL( signal_close() )        , areaSelector, SLOT( close() ) );
    connect(this , SIGNAL( signal_show(bool) )     , areaSelector, SLOT( setVisible( bool ) ) );
    connect(this , SIGNAL( signal_recording(bool) ), areaSelector, SLOT( slot_recordMode(bool) ) );
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

void MainWindow::enable_or_disable_tabs(bool val){
    QList<QWidget*>list = ui->tab_1->findChildren<QWidget*>();
    QList<QWidget*>list2 = ui->tab_2->findChildren<QWidget*>();
    list.append(list2);
    foreach(QWidget* w, list){
      w->setEnabled(val);
    }
}

void MainWindow::on_pushButtonSelectArea_clicked()
{
    static bool first_call=true;
    bool state = ui->pushButtonSelectArea->isChecked();
    qDebug()<<"select area clicked. state: "<<state;
    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    if(state){
        if(first_call){
            first_call=false;
            areaSelector->slot_init();
        }
        qDebug()<<"Lanciato il segnale di selezione";
        emit signal_show(true);
    }
}

void MainWindow::on_pushButtonFullscreen_clicked()
{
    qDebug()<<"fullscreen button clicked";
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);
    qDebug()<<"area selector is visible: "<<areaSelector->isVisible();
    emit signal_show(false);
}

void MainWindow::on_toolButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory", QDir::homePath());
    ui->lineEditPath->setText(path);
}

void MainWindow::on_pushButtonStart_clicked()
{
    if(ui->pushButtonFullscreen->isChecked() | ui->pushButtonSelectArea->isChecked() ){
        ui->pushButtonStop->setEnabled(true);
        ui->pushButtonPause->setEnabled(true);
        ui->pushButtonStart->setDisabled(true);
        enable_or_disable_tabs(false);
        if(ui->checkBoxMinimize->isChecked()) QWidget::showMinimized();
     if(ui->pushButtonSelectArea->isChecked()) emit signal_recording(true);
    }else{
        return;
    }
}

void MainWindow::on_pushButtonPause_clicked()
{
    ui->pushButtonResume->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
}

void MainWindow::on_pushButtonResume_clicked()
{
    ui->pushButtonPause->setEnabled(true);
    ui->pushButtonResume->setEnabled(false);
}

void MainWindow::on_pushButtonStop_clicked()
{
    enable_or_disable_tabs(true);
    ui->pushButtonStart->setEnabled(true);
    if(ui->pushButtonSelectArea->isChecked()){
        emit signal_recording(false);
    }

    ui->pushButtonPause->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);
    ui->lineEditPath->setText(QDir::homePath());
    ui->radioButtonYes->setChecked(true);
    ui->radioButton24->setChecked(true);

}
