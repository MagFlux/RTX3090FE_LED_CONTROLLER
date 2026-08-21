#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // When launched from the Windows shell:startup folder we pass
    // `--background` so the app comes up in the tray instead of the foreground.
    bool startHidden = a.arguments().contains("--background");

    MainWindow w(startHidden);
    if (!startHidden) {
        w.show();
    }
    return a.exec();
}