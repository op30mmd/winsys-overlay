#include "overlaywidget.h"

#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    // Enable Qt logging for debugging
    QLoggingCategory::setFilterRules("qt.qpa.plugin.debug=true");
    
    QApplication a(argc, argv);
    
    // Debug: Print Qt and application information
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    qDebug() << "Working directory:" << QDir::currentPath();
    qDebug() << "Library paths:" << QCoreApplication::libraryPaths();
    
    // Set Qt plugin path to help find platform plugins
    QString appDir = QCoreApplication::applicationDirPath();
    QCoreApplication::addLibraryPath(appDir);
    QCoreApplication::addLibraryPath(appDir + "/platforms");
    
    qDebug() << "Updated library paths:" << QCoreApplication::libraryPaths();
    
    // Check for platform plugins
    QDir platformsDir(appDir + "/platforms");
    if (platformsDir.exists()) {
        qDebug() << "Platforms directory exists";
        qDebug() << "Platform plugins found:" << platformsDir.entryList(QStringList() << "*.dll", QDir::Files);
    } else {
        qDebug() << "ERROR: Platforms directory does not exist!";
    }
    
    // Check for required Qt DLLs
    QStringList requiredDlls = {"Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll"};
    for (const QString& dll : requiredDlls) {
        QFileInfo dllInfo(appDir + "/" + dll);
        qDebug() << dll << (dllInfo.exists() ? "found" : "MISSING");
    }
    
    QCoreApplication::setOrganizationName("WinSysOverlay");
    QCoreApplication::setApplicationName("WinSys-Overlay");

    OverlayWidget w;
    w.show();

    return a.exec();
}
