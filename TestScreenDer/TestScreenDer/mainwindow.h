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

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
