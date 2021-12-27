#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AreaSelector.h"
#include "ScreenRecorder.h"
#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QShortcut>
#include <QMessageBox>
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
    QMessageBox errorDialog;

private:
    //settings chosen by the user in the window
    RecordingRegionSettings rrs;
    VideoSettings vs;
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

    void on_radioButtonYes_clicked();

    void on_radioButtonNo_clicked();

    void on_radioButton24_clicked();

    void on_radioButton30_clicked();

    void on_radioButton60_clicked();

    void on_horizontalSlider_sliderMoved(int position);

    void on_lineEditPath_textEdited(const QString &arg1);

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
    QScreen *screen;

    void enable_or_disable_tabs(bool);
    void setDefaultValues();
};
#endif // MAINWINDOW_H
