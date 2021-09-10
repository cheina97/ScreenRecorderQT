#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "AreaSelector.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    AreaSelector *areaSelector;

private slots:
    void on_pushButtonSelectArea_clicked();
    void on_pushButtonFullscreen_clicked();
    void on_toolButton_clicked();

    void on_pushButtonStart_clicked();

    void on_pushButtonPause_clicked();

    void on_pushButtonResume_clicked();

    void on_pushButtonStop_clicked();

    void on_minimize_toggled(bool checked);

signals:
    void signal_close();
    void signal_selection();
    void signal_recording(bool);
    void signal_reset_areaSelector();
protected:
    void closeEvent( QCloseEvent *event );

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
    bool minimizeInSysTray;


    void enable_or_disable_tabs(bool);
};
#endif // MAINWINDOW_H
