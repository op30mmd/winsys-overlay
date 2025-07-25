#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QPoint>
#include "sysinfomonitor.h"

class QLabel;

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    OverlayWidget(QWidget *parent = nullptr);
    ~OverlayWidget();

public slots:
    void updateStats(const SysInfo& info);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QLabel *m_cpuLabel;
    QLabel *m_memLabel;
    SysInfoMonitor *m_monitor;
    QPoint m_dragPosition;
};
#endif // OVERLAYWIDGET_H