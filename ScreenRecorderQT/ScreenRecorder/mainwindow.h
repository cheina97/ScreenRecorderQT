#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AreaSelector.h"
#include "ScreenRecorder.h"
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QShortcut>
#include <QSystemTrayIcon>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    AreaSelector *areaSelector;
    ScreenRecorder *screenRecorder;

private:
    //settings chosen by the user in the window
    RecordingRegionSettings rrs;
    VideoSettings vs;
    bool audioOn;
    string outFilePath;

  private slots:
    void on_pushButtonSelectArea_clicked();
    void on_pushButtonFullscreen_clicked();
    void on_toolButton_clicked();
    void on_pushButtonStart_clicked();
    void on_pushButtonPause_clicked();
    void on_pushButtonResume_clicked();
    void on_pushButtonStop_clicked();
    void on_checkBoxMinimize_toggled(bool);

  signals:
    void signal_close();
    void signal_show(bool);
    void signal_recording(bool);

  protected:
    void closeEvent(QCloseEvent *event);

  private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    void createActions();
    void createTrayIcon();
    QAction *showhideAction;
    QAction *startAction;
    QAction *resumeAction;
    QAction *pauseAction;
    QAction *stopAction;
    QAction *quitAction;
    QShortcut *startstop_shortcut;
    bool minimizeInSysTray;

    void enable_or_disable_tabs(bool);
};
#endif // MAINWINDOW_H
