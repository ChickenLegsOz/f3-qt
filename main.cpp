#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Set application-wide icon
    QIcon appIcon(":/icon/f3.png");
    a.setWindowIcon(appIcon);
    
    // Create and configure main window
    MainWindow w;
    w.setWindowIcon(appIcon);  // Explicitly set icon for main window
    w.show();

    return a.exec();
}
