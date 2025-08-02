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
    void createIcons();
    void createLayout();
    void updateLayoutOrientation();
    QPixmap createColoredIcon(const QString& iconPath, const QColor& color, const QSize& size = QSize(16, 16));

    // Original labels
    QLabel *m_cpuLabel;
    QLabel *m_memLabel;
    QLabel *m_ramLabel;
    QLabel *m_diskLabel;
    QLabel *m_gpuLabel;
    
    // New metric labels
    QLabel *m_fpsLabel;
    QLabel *m_netDownLabel;
    QLabel *m_netUpLabel;
    QLabel *m_dailyDataLabel;
    QLabel *m_cpuTempLabel;
    QLabel *m_gpuTempLabel;
    QLabel *m_processesLabel;
    QLabel *m_uptimeLabel;
    
    SysInfoMonitor *m_monitor;
    QPoint m_dragPosition;
    
    // Icon pixmaps
    QPixmap m_cpuIcon;
    QPixmap m_memIcon;
    QPixmap m_ramIcon;
    QPixmap m_diskIcon;
    QPixmap m_gpuIcon;
    QPixmap m_fpsIcon;
    QPixmap m_netDownIcon;
    QPixmap m_netUpIcon;
    QPixmap m_dailyDataIcon;
    QPixmap m_cpuTempIcon;
    QPixmap m_gpuTempIcon;
    QPixmap m_processesIcon;
    QPixmap m_uptimeIcon;
    
    // Container widgets for better management
    QWidget *m_cpuWidget;
    QWidget *m_memWidget;
    QWidget *m_ramWidget;
    QWidget *m_diskWidget;
    QWidget *m_gpuWidget;
    QWidget *m_fpsWidget;
    QWidget *m_netDownWidget;
    QWidget *m_netUpWidget;
    QWidget *m_dailyDataWidget;
    QWidget *m_cpuTempWidget;
    QWidget *m_gpuTempWidget;
    QWidget *m_processesWidget;
    QWidget *m_uptimeWidget;
    
    // Icon labels for updating icons
    QLabel *m_cpuIconLabel;
    QLabel *m_memIconLabel;
    QLabel *m_ramIconLabel;
    QLabel *m_diskIconLabel;
    QLabel *m_gpuIconLabel;
    QLabel *m_fpsIconLabel;
    QLabel *m_netDownIconLabel;
    QLabel *m_netUpIconLabel;
    QLabel *m_dailyDataIconLabel;
    QLabel *m_cpuTempIconLabel;
    QLabel *m_gpuTempIconLabel;
    QLabel *m_processesIconLabel;
    QLabel *m_uptimeIconLabel;
};
#endif // OVERLAYWIDGET_H