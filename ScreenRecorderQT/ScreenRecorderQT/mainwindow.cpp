#include "mainwindow.h"

#include <QAudioDeviceInfo>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <condition_variable>
#include <mutex>

#include "AreaSelector.h"
#include "GetAudioDevices.h"
#include "ScreenRecorder.h"
#include "ui_mainwindow.h"

std::unique_ptr<ScreenRecorder> screenRecorder;
RecordingRegionSettings rrs;
VideoSettings vs;
std::string outFilePath;
std::string deviceName;

// Used to syncronize the record ending and the buttons enabling
mutex m;
condition_variable cv;

void MainWindow::alignValues() {
    ///rrs values
    if (ui->pushButtonFullscreen->isChecked()) {
        rrs.width = screen->size().width();
        rrs.height = screen->size().height();
        rrs.offset_x = 0;
        rrs.offset_y = 0;

        rrs.screen_number = 0;
    } else {
        rrs.height = areaSelector->getHeight();
        rrs.width = areaSelector->getWidth();
        rrs.offset_x = areaSelector->getX();
        rrs.offset_y = areaSelector->getY();
    }

    ///vs values
    if (ui->radioButton30->isChecked())
        vs.fps = 30;
    else if (ui->radioButton24->isChecked())
        vs.fps = 24;
    else if (ui->radioButton60->isChecked())
        vs.fps = 60;

    setQualityANDCompression(ui->horizontalSlider->value());

    vs.audioOn = ui->radioButtonYes->isChecked();
    outFilePath = ui->lineEditPath->text().toStdString();

    deviceName = ui->comboBox->currentText().toStdString();
    minimizeInSysTray = ui->checkBoxMinimize->isChecked();
}

void MainWindow::setQualityANDCompression(int position) {
    if (position == 1) {
        vs.quality = 0.3;
        vs.compression = 8;
    } else if (position == 2) {
        vs.quality = 0.5;
        vs.compression = 8;
    } else if (position == 3) {
        vs.quality = 1;
        vs.compression = 8;
    } else if (position == 4) {
        vs.quality = 1;
        vs.compression = 7;
    } else if (position == 5) {
        vs.quality = 1;
        vs.compression = 6;
    } else if (position == 6) {
        vs.quality = 1;
        vs.compression = 5;
    } else if (position == 7) {
        vs.quality = 1;
        vs.compression = 4;
    }
}

void MainWindow::defaultButtonProperties() {
    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonPause->setDisabled(true);
    ui->pushButtonResume->setDisabled(true);
    ui->pushButtonStop->setDisabled(true);
    startAction->setDisabled(false);
    pauseAction->setDisabled(true);
    resumeAction->setDisabled(true);
    stopAction->setDisabled(true);
    quitAction->setEnabled(true);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
}

void MainWindow::setGeneralDefaultProperties() {
    //enable tabs
    enable_or_disable_tabs(true);

    //button properties
    defaultButtonProperties();

    // line edit default text
    //ui->lineEditPath->setText(QDir::homePath());

    // radio buttons
    ui->radioButton30->setChecked(true);

    // slider
    ui->horizontalSlider->setValue(7);
}

void MainWindow::showOrHideWindow(bool recording) {
    if (!recording) {
        if (minimizeInSysTray)
            show();
        trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
    } else {
        if (minimizeInSysTray)
            hide();
        trayIcon->setIcon(QIcon(":/icons/trayicon_recording.png"));
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    screen = QGuiApplication::primaryScreen();

    ui->setupUi(this);

    ui->verticalLayout_6->addWidget(ui->comboBox);

    ui->lineEditPath->setText(QString::fromStdString(forge_outpath(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation).toStdString())));

    areaSelector = new AreaSelector();

    setWindowTitle("Screen Recorder QT");

    errorDialog.setFixedSize(500, 200);

    // audio default selection
    ui->radioButtonYes->setChecked(true);

    // tab widget
    ui->tabWidget->setCurrentIndex(0);  // always open on first tab

    // button properties
    ui->pushButtonFullscreen->setCheckable(true);
    ui->pushButtonFullscreen->setChecked(true);
    ui->pushButtonSelectArea->setCheckable(true);

    ui->pushButtonFullscreen->setIcon(QIcon(":/icons/fullscreen.png"));
    ui->pushButtonSelectArea->setIcon(QIcon(":/icons/area.png"));

    //radio button tooltip
    ui->radioButton60->setMouseTracking(true);
    ui->radioButton60->setToolTip("High performances required");

    // slider properties
    ui->horizontalSlider->setTracking(true);
    ui->horizontalSlider->setMinimum(1);
    ui->horizontalSlider->setMaximum(7);

    //options in the combobox
#if defined _WIN32
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos)
        ui->comboBox->addItem(tr(deviceInfo.deviceName().toStdString().c_str()));
        //deviceInfo.deviceName() is a QString but the addItem function needs a char*.
        //there is no viable conversion from QString to char* so the conversion is: QString->std::string->char*
#endif
#if defined __linux__
    for (auto &device : getAudioDevices()) {
        ui->comboBox->addItem(tr(device.c_str()));
    }
#endif

    //default values for screen recorder object:
    alignValues();

    // connect
    connect(this, SIGNAL(signal_close()), areaSelector, SLOT(close()));
    connect(this, SIGNAL(signal_show(bool)), areaSelector,
            SLOT(setVisible(bool)));
    connect(this, SIGNAL(signal_recording(bool)), areaSelector,
            SLOT(slot_recordMode(bool)));
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        createActions();
        createTrayIcon();
    }

    //set the general properties for the elements of the window
    setGeneralDefaultProperties();
}

MainWindow::~MainWindow() {
    delete ui;
}

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

string MainWindow::forge_outpath(string outFilePath) {
    std::string ending = ".mp4";
    if (!std::equal(ending.rbegin(), ending.rend(), outFilePath.rbegin())) {
        ending = "/";
        if (std::equal(ending.rbegin(), ending.rend(), outFilePath.rbegin()))
            outFilePath += "out.mp4";
        else {
            outFilePath += "/out.mp4";
        }
    }
    return outFilePath;
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

    qDebug() << "rrs.fullscreen = false";

    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    //avoid bad behaviour in case of double click
    if (state) {
        if (first_call) {
            first_call = false;
            areaSelector->slot_init();
        }
        emit signal_show(true);
    }
}

void MainWindow::on_pushButtonFullscreen_clicked() {
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);

    rrs.width = screen->size().width();
    rrs.height = screen->size().height();
    rrs.offset_x = 0;
    rrs.offset_y = 0;

    qDebug() << "fullscreen: " << rrs.width << "x" << rrs.height;

    emit signal_show(false);
}

void MainWindow::on_toolButton_clicked() {
    QString path = QFileDialog::getSaveFileName(this, tr("Choose file to save"), QString(QDir::homePath()), tr("Audio (*.mp4)"));
    path = path.endsWith(".mp4") ? path : path + ".mp4";
    ui->lineEditPath->setText(path);

    outFilePath = path.toStdString();
    qDebug() << "outFilePath: " << QString::fromStdString(outFilePath);
}

//////MAIN ACTIONS//////
void MainWindow::on_pushButtonStart_clicked() {
    if (ui->lineEditPath->text().isEmpty()) {
        QMessageBox::information(this, tr("Invalid Path"),
                                 tr("The message box containing the path where to save the file cannot be empty"));
        return;
    }
    /* if (ui->pushButtonSelectArea->isChecked()) {
        rrs.height = areaSelector->getHeight();
        rrs.width = areaSelector->getWidth();
        rrs.offset_x = areaSelector->getX();
        rrs.offset_y = areaSelector->getY();
    } */

    ui->pushButtonStop->setEnabled(true);
    stopAction->setEnabled(true);
    ui->pushButtonPause->setEnabled(true);
    pauseAction->setEnabled(true);
    ui->pushButtonStart->setDisabled(true);
    startAction->setDisabled(true);
    quitAction->setDisabled(true);
    enable_or_disable_tabs(false);

    ui->lineEditPath->setText(QString(outFilePath.c_str()));

    showOrHideWindow(true);

    if (ui->pushButtonSelectArea->isChecked())
        emit signal_recording(true);  //this changes the color of the border

    alignValues();

    qDebug() << "Values rrs: \n wxh: " << rrs.width << " x " << rrs.height << "\noffset: " << rrs.offset_x << ", " << rrs.offset_y
             << "\n screen: " << rrs.screen_number << "\n fullscreen: " << ui->pushButtonFullscreen->isChecked() << "\n";
    qDebug() << "values di vs:"
             << "\n fps: " << vs.fps << "\n quality: " << vs.quality << "\n compression: " << vs.compression << "\n audio: " << QString::number(vs.audioOn) << "\n";
    qDebug() << "Directory: " << QString::fromStdString(outFilePath);
    qDebug() << "DeviceName: " << QString::fromStdString(deviceName);
    qDebug() << "minimize:: " << minimizeInSysTray;
    try {
        screenRecorder = make_unique<ScreenRecorder>(rrs, vs, outFilePath, deviceName);
        std::cout << "Built ScreenRecorder Object" << std::endl;
        auto record_thread = std::thread{[&]() {
            try {
                std::cout << "-> RECORDING..." << std::endl;
                screenRecorder->record();
            } catch (const std::exception &e) {
                setGeneralDefaultProperties();
                alignValues();
                std::string message = e.what();
                errorDialog.critical(0, "Error", QString::fromStdString(message));
            }
            screenRecorder.reset();
            cv.notify_one();
        }};
        record_thread.detach();
    } catch (const std::exception &e) {
        setGeneralDefaultProperties();
        alignValues();

        // Call to open the error dialog
        std::string message = e.what();
        errorDialog.critical(0, "Error", QString::fromStdString(message));
    }
}

void MainWindow::on_pushButtonPause_clicked() {
    screenRecorder->pauseRecording();
    ui->pushButtonResume->setEnabled(true);
    resumeAction->setEnabled(true);
    ui->pushButtonPause->setEnabled(false);
    pauseAction->setEnabled(false);
    trayIcon->setIcon(QIcon(":/icons/trayicon_normal.png"));
}

void MainWindow::on_pushButtonResume_clicked() {
    screenRecorder->resumeRecording();
    ui->pushButtonPause->setEnabled(true);
    pauseAction->setEnabled(true);
    ui->pushButtonResume->setEnabled(false);
    resumeAction->setEnabled(false);
    showOrHideWindow(true);
}

void MainWindow::on_pushButtonStop_clicked() {
    if (screenRecorder) screenRecorder.get()->stopRecording();
    if (ui->pushButtonSelectArea->isChecked()) {
        emit signal_recording(false);
    }
    pauseAction->setDisabled(true);
    stopAction->setDisabled(true);
    ui->pushButtonPause->setEnabled(false);
    ui->pushButtonStop->setEnabled(false);

    auto waiting_thread = std::thread{[&]() {
        unique_lock ul{m};
        cv.wait(ul, []() { return !screenRecorder; });
        defaultButtonProperties();
        showOrHideWindow(false);
        enable_or_disable_tabs(true);
    }};
    waiting_thread.detach();
}

////Settings
void MainWindow::on_checkBoxMinimize_toggled(bool checked) {
    minimizeInSysTray = checked;
}

void MainWindow::on_radioButtonYes_clicked() {
    vs.audioOn = true;
    ui->comboBox->setDisabled(false);
}

void MainWindow::on_radioButtonNo_clicked() {
    vs.audioOn = false;
    ui->comboBox->setDisabled(true);
}

void MainWindow::on_radioButton24_clicked() {
    vs.fps = 24;
}

void MainWindow::on_radioButton30_clicked() {
    vs.fps = 30;
}

void MainWindow::on_radioButton60_clicked() {
    vs.fps = 60;
}

void MainWindow::on_horizontalSlider_sliderMoved(int position) {
    setQualityANDCompression(position);
}

void MainWindow::on_lineEditPath_textEdited(const QString &arg1) {
    outFilePath = forge_outpath(arg1.toStdString());
}

void MainWindow::on_comboBox_activated(const QString &arg1) {
    deviceName = arg1.toStdString();
}
