#include "overlaywidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    OverlayWidget w;
    w.show();
    return a.exec();
}