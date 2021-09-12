#include "mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>

#include "AreaSelector.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    areaSelector = new AreaSelector();

    setWindowTitle("ScreenCapture");

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

    // connect
    connect(this, SIGNAL(signal_close()), areaSelector, SLOT(close()));
    // connect(this, SIGNAL(signal_selection()), areaSelector,
    // SLOT(slot_init())); connect(ui->pushButtonSelectArea,
    // SIGNAL(toggled(bool)), areaSelector,
    //        SLOT(setVisible(bool)));
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
    qDebug() << "select area clicked. state: " << state;
    ui->pushButtonFullscreen->setChecked(false);
    ui->pushButtonSelectArea->setChecked(true);
    if (state) {
        if (first_call) {
            first_call = false;
            areaSelector->slot_init();
        }
        qDebug() << "Lanciato il segnale di selezione";
        // emit signal_selection();
        areaSelector->setVisible(true);
    }
}

void MainWindow::on_pushButtonFullscreen_clicked() {
    qDebug() << "fullscreen button clicked";
    ui->pushButtonSelectArea->setChecked(false);
    ui->pushButtonFullscreen->setChecked(true);
    qDebug() << "area selector is visible: " << areaSelector->isVisible();
    // emit signal_reset_areaSelector();
    areaSelector->setVisible(false);
}

void MainWindow::on_toolButton_clicked() {
    QString path = QFileDialog::getExistingDirectory(this, "Select a directory",
                                                     QDir::homePath());
    ui->lineEditPath->setText(path);
}

void MainWindow::on_pushButtonStart_clicked() {
    if (ui->pushButtonFullscreen->isChecked() |
        ui->pushButtonSelectArea->isChecked()) {
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
            emit signal_recording(true);
        trayIcon->setIcon(QIcon(":/icons/trayicon_recording.png"));
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
}

void MainWindow::on_minimize_toggled(bool checked) {
    minimizeInSysTray = checked;
}
