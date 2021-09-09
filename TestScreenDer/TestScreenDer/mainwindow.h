#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

signals:
    void signal_close();
    void signal_selection();
    void signal_recording(bool);

protected:
    void closeEvent( QCloseEvent *event );

private:
    Ui::MainWindow *ui;
    void enable_or_disable_tabs(bool);
};
#endif // MAINWINDOW_H
