#include "mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QAudioDeviceInfo>

#include "AreaSelector.h"
#include "ScreenRecorder.h"
#include "ui_mainwindow.h"

ScreenRecorder *screenRecorder;
RecordingRegionSettings rrs;
VideoSettings vs;
string outFilePath;

void MainWindow::setDefaultValues(){
    ///rrs values
    screen = QGuiApplication::primaryScreen();
    rrs.width = screen->size().width();
    rrs.height = screen->size().height();
    rrs.offset_x = 0;
    rrs.offset_y = 0;
    //CHECK: dovrebbe cambiare?
    rrs.screen_number = 0;
    rrs.fullscreen = true;

    ///vs vlaues
    vs.fps = 24;
    vs.quality = 0.6;
    vs.compression = 4;
    vs.audioOn = true;
    outFilePath = QDir::homePath().toStdString()+"/out.mp4";

    deviceName = ui->comboBox->currentText().toStdString();
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    areaSelector = new AreaSelector();

    setWindowTitle("ScreenCapture");

    errorDialog.setFixedSize(500, 200);

    // tab widget
    ui->tabWidget->setCurrentIndex(0); // always open on first tab

    // button properties
    ui->pushButtonFullscreen->setCheckable(true);
    ui->pushButtonFullscreen->setChecked(true);

    ui->pushButtonFullscreen->setIcon(QIcon(":/icons/fullscreen.png"));

    ui->pushButtonSelectArea->setCheckable(true);
    ui->pushButtonSelectArea->setIcon(QIcon(":/icons/area.png"));

    ui->pushButtonPause->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);

    // line edit default text
    ui->lineEditPath->setText(QDir::homePath());

    // audio default selection
    ui->radioButtonYes->setChecked(true);

    // radio buttons
    ui->radioButton24->setChecked(true);
    ui->radioButton60->setMouseTracking(true);
    ui->radioButton60->setToolTip("High performances required");

    // checkbox properties
    ui->checkBoxMinimize->setChecked(true);

    // slider properties
    ui->horizontalSlider->setTracking(true);
    ui->horizontalSlider->setMinimum(1);
    ui->horizontalSlider->setMaximum(3);
    ui->horizontalSlider->setValue(2);


   //options in the combobox
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
        ui->comboBox->addItem(tr(deviceInfo.deviceName().toStdString().c_str()));
    //deviceInfo.deviceName() is a QString but the addItem function needs a char*.
    //there is no viable conversion from QString to char* so the conversion is: QString->std::string->char*

    //default values for screen recorder object:
    setDefaultValues();

    // connect
    connect(this, SIGNAL(signal_close()), areaSelector, SLOT(close()));
    connect(this, SIGNAL(signal_show(bool)), areaSelector,
            SLOT(setVisible(bool)));
    connect(this, SIGNAL(signal_recording(bool)), areaSelector,
            SLOT(slot_recordMode(bool)));
    minimizeInSysTray = false;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createActions();
        createTrayIcon();
    }

    startstop_shortcut = new QShortcut{QKeySequence(tr("Ctrl+O")), this};
    connect(startstop_shortcut, &QShortcut::activated, this,
            [&]() { qDebug() << "Pressed shortcut"; });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::createActions() {
    showhideAction = new QAction(tr("Show/Hide"), this);
    connect(showhideAction, &QAction::triggered, this, [&]() {
        if (this->isHidden()) {
            this->show();
        } else {
            this->hide();
        }
    });

    startAction = new QAction(tr("Start"), this);
    connect(startAction, &QAction::triggered, this,
            [&]() { on_pushButtonStart_clicked(); });

    resumeAction = new QAction(tr("Resume"), this);
    connect(resumeAction, &QAction::triggered, this,
            [&]() { on_pushButtonResume_clicked(); });
    resumeAction->setEnabled(false);

    pauseAction = new QAction(tr("Pause"), this);
    connect(pauseAction, &QAction::triggered, this,
            [&]() { on_pushButtonPause_clicked(); });
    pauseAction->setEnabled(false);

    stopAction = new QAction(tr("Stop"), this);
    connect(stopAction, &QAction::triggered, this,
            [&]() { on_pushButtonStop_clicked(); });
    stopAction->setEnabled(false);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, [&]() {
        emit signal_close();
        QCoreApplication::quit();
    });
}

void MainWindow::createTrayIcon() {
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showhideAction);
    trayIconMenu->addAction(startAction);
    trayIconMenu->addAction(resumeAction);
    trayIconMenu->addAction(pauseAction);
    trayIconMenu->addAction(stopAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
    trayIcon->setVisible(true);
}

void MainWindow::closeEvent(QCloseEvent *event) {
#ifdef Q_OS_MACOS
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        QMessageBox::information(this, tr("Systray"),
                                 tr("The program will keep running in the "
                                    "system tray. To terminate the program, "
                                    "choose <b>Quit</b> in the context menu "
                                    "of the system tray entry."));
        hide();
        event->ignore();
    } else {
        emit signal_close();
        QCoreApplication::quit();
    }
}

void MainWindow::enable_or_disable_tabs(bool val) {
    QList<QWidget *> list = ui->tab_1->findChildren<QWidget *>();
    QList<QWidget *> list2 = ui->tab_2->findChildren<QWidget *>();
    list.append(list2);
    foreach (QWidget *w, list) { w->setEnabled(val); }
}

void MainWindow::on_pushButtonSelectArea_clicked() {
    static bool first_call = true;
    bool state = ui->pushButtonSelectArea->isChecked();

    rrs.fullscreen = false;
    qDebug()<<"rrs.fullscreen = false";

    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    //evitare bad behaviour in caso di doppio click
    if (state) {
        if (first_call) {
            first_call = false;
            areaSelector->slot_init();
        }
        emit signal_show(true);
    }
}

void MainWindow::on_pushButtonFullscreen_clicked() {
    qDebug() << "fullscreen button clicked";
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);

    rrs.fullscreen = true;
    rrs.width = screen->size().width();
    rrs.height = screen->size().height();
    rrs.offset_x = 0;
    rrs.offset_y = 0;
    qDebug()<<"rrs.fullscreen = true";

    emit signal_show(false);
}

void MainWindow::on_toolButton_clicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory", QDir::homePath());
    ui->lineEditPath->setText(path);

    outFilePath = path.toStdString()+"/out.mp4";
    qDebug()<<"outFilePath: "<<QString::fromStdString(outFilePath);
}

//////MAIN ACTIONS//////
void MainWindow::on_pushButtonStart_clicked() {

    if (ui->pushButtonFullscreen->isChecked() | ui->pushButtonSelectArea->isChecked()) {

        if(!rrs.fullscreen){
            rrs.height = areaSelector->getHeight();
            rrs.width = areaSelector->getWidth();
            rrs.offset_x = areaSelector->getX();
            rrs.offset_y = areaSelector->getY();
        }

        ui->pushButtonStop->setEnabled(true);
        stopAction->setEnabled(true);
        ui->pushButtonPause->setEnabled(true);
        pauseAction->setEnabled(true);
        ui->pushButtonStart->setDisabled(true);
        startAction->setDisabled(true);
        enable_or_disable_tabs(false);
        if (minimizeInSysTray)
            hide();
        if (ui->pushButtonSelectArea->isChecked())
            emit signal_recording(true); //this changes the color of the border
        trayIcon->setIcon(QIcon(":/icons/trayicon_recording.png"));

        qDebug()<<"Valori rrs: "<<Qt::endl<<"wxh: "<<rrs.width<<" x "<<rrs.height<<Qt::endl<<"offset: "<<rrs.offset_x<<", "<<rrs.offset_y<<Qt::endl
               <<"screen: "<<rrs.screen_number<<Qt::endl<<"fullscreen: "<< rrs.fullscreen<<Qt::endl;
        qDebug()<<"valori di vs:"<<Qt::endl<<"fps: "<<vs.fps<<Qt::endl<<"quality: "<<vs.quality<<Qt::endl<<"audio: "<<QString::number(vs.audioOn)<<Qt::endl;
        qDebug()<<"Directory: "<<QString::fromStdString(outFilePath);
        qDebug()<<"DeviceName: "<<QString::fromStdString(deviceName);
        try{
            vs.capturetime_seconds= 5;
            screenRecorder = new ScreenRecorder(rrs, vs, outFilePath, deviceName);
            cout << "-> Costruito oggetto Screen Recorder" << endl;
            cout << "-> RECORDING..." << endl;
            screenRecorder->record();
        } catch(const exception &e){
            // Call to open the error dialog
            string message = e.what();
            message += "\nPlease close and restart the application";
            errorDialog.critical(0, "Error", QString::fromStdString(message));
        }
    }
}

void MainWindow::on_pushButtonPause_clicked() {
    ui->pushButtonResume->setEnabled(true);
    resumeAction->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
    pauseAction->setEnabled(false);
    if (minimizeInSysTray)
        show();
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
}

void MainWindow::on_pushButtonResume_clicked() {
    ui->pushButtonPause->setEnabled(true);
    pauseAction->setEnabled(true);
    ui->pushButtonResume->setEnabled(false);
    resumeAction->setEnabled(false);
    if (minimizeInSysTray)
        hide();
    trayIcon->setIcon(QIcon(":/icons/trayicon_recording.png"));
}

void MainWindow::on_pushButtonStop_clicked() {
    enable_or_disable_tabs(true);
    ui->pushButtonStart->setEnabled(true);
    startAction->setEnabled(true);
    if (ui->pushButtonSelectArea->isChecked()) {
        emit signal_recording(false);
    }

    ui->pushButtonPause->setDisabled(true);
    pauseAction->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    resumeAction->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);
    stopAction->setDisabled(true);
    ui->lineEditPath->setText(QDir::homePath());
    ui->radioButtonYes->setChecked(true);
    ui->radioButton24->setChecked(true);
    if (minimizeInSysTray)
        show();
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));

    setDefaultValues();
}

////Settings
void MainWindow::on_checkBoxMinimize_toggled(bool checked) {
    minimizeInSysTray = checked;
}

void MainWindow::on_radioButtonYes_clicked(){
    vs.audioOn = true;
    ui->comboBox->setDisabled(false);
}

void MainWindow::on_radioButtonNo_clicked()
{
    vs.audioOn = false;
    ui->comboBox->setDisabled(true);
}

void MainWindow::on_radioButton24_clicked()
{
    vs.fps = 24;
}

void MainWindow::on_radioButton30_clicked()
{
    vs.fps = 30;
}

void MainWindow::on_radioButton60_clicked()
{
    vs.fps = 60;
}

void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    if(position == 1){
        vs.quality = 0.3;
        vs.compression = 8;
    }else if(position == 2){
        vs.quality = 0.6;
        vs.compression = 4;
    }else{
        vs.quality = 1;
        vs.compression = 5;
    }
}

void MainWindow::on_lineEditPath_textEdited(const QString &arg1)
{
    outFilePath = arg1.toStdString()+"/out.mp4";
}


void MainWindow::on_comboBox_activated(const QString &arg1)
{
    deviceName = arg1.toStdString();
}

