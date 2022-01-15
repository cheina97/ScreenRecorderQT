#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    QMessageBox errorDialog;
    try {
        MainWindow w;
        w.show();
        return a.exec();
    } catch (const std::exception& e) {
        // Call to open the error dialog
        std::string message = e.what();
        message += "\nPlease close and restart the application";
        errorDialog.critical(0, "Error", QString::fromStdString(message));
    }
}
