#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QPoint>
#include "sysinfomonitor.h"

class QLabel;
class QMouseEvent;
class QContextMenuEvent;

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
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void openSettingsDialog();
    void applySettings();

private:
    void loadSettings();
    void setupUi();

    QLabel *m_cpuLabel;
    QLabel *m_memLabel;
    QLabel *m_ramLabel;
    QLabel *m_diskLabel;
    QLabel *m_gpuLabel;
    SysInfoMonitor *m_monitor;
    QPoint m_dragPosition;
};
#endif // OVERLAYWIDGET_H