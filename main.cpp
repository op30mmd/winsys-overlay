#include "overlaywidget.h"

#include <QApplication>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // Set organization and application name for QSettings
    QCoreApplication::setOrganizationName("WinSysOverlay");
    QCoreApplication::setApplicationName("WinSys-Overlay");

    OverlayWidget w;
    w.show();
    return a.exec();
}