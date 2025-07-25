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

    QLabel *m_cpuLabel;
    QLabel *m_memLabel;
    QLabel *m_ramLabel;
    QLabel *m_diskLabel;
    QLabel *m_gpuLabel;
    SysInfoMonitor *m_monitor;
    QPoint m_dragPosition;
    
    // Icon pixmaps
    QPixmap m_cpuIcon;
    QPixmap m_memIcon;
    QPixmap m_ramIcon;
    QPixmap m_diskIcon;
    QPixmap m_gpuIcon;
    
    // Container widgets for better management
    QWidget *m_cpuWidget;
    QWidget *m_memWidget;
    QWidget *m_ramWidget;
    QWidget *m_diskWidget;
    QWidget *m_gpuWidget;
    
    // Icon labels for updating icons
    QLabel *m_cpuIconLabel;
    QLabel *m_memIconLabel;
    QLabel *m_ramIconLabel;
    QLabel *m_diskIconLabel;
    QLabel *m_gpuIconLabel;
};
#endif // OVERLAYWIDGET_H