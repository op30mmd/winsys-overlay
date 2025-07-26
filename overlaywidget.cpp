#include "overlaywidget.h"
#include "settingsdialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QApplication>
#include <QSettings>
#include <QPainter>
#include <QStyle>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent)
{
    // Make the window frameless, always on top, and transparent
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_monitor = new SysInfoMonitor(this);

    setupUi();
    createIcons();
    createLayout();
    loadSettings();

    connect(m_monitor, &SysInfoMonitor::statsUpdated, this, &OverlayWidget::updateStats);
}

void OverlayWidget::setupUi()
{
    // Original labels
    m_cpuLabel = new QLabel("CPU: ...", this);
    m_memLabel = new QLabel("MEM: ...", this);
    m_ramLabel = new QLabel("RAM: ...", this);
    m_diskLabel = new QLabel("DSK: ...", this);
    m_gpuLabel = new QLabel("GPU: ...", this);
    
    // New metric labels
    m_fpsLabel = new QLabel("FPS: ...", this);
    m_netDownLabel = new QLabel("↓: ...", this);
    m_netUpLabel = new QLabel("↑: ...", this);
    m_dailyDataLabel = new QLabel("Daily: ...", this);
    m_cpuTempLabel = new QLabel("CPU°: ...", this);
    m_gpuTempLabel = new QLabel("GPU°: ...", this);
    m_processesLabel = new QLabel("Proc: ...", this);
    m_uptimeLabel = new QLabel("Up: ...", this);
    
    // Initialize container widgets to nullptr
    m_cpuWidget = nullptr;
    m_memWidget = nullptr;
    m_ramWidget = nullptr;
    m_diskWidget = nullptr;
    m_gpuWidget = nullptr;
    m_fpsWidget = nullptr;
    m_netDownWidget = nullptr;
    m_netUpWidget = nullptr;
    m_dailyDataWidget = nullptr;
    m_cpuTempWidget = nullptr;
    m_gpuTempWidget = nullptr;
    m_processesWidget = nullptr;
    m_uptimeWidget = nullptr;
}

void OverlayWidget::createIcons()
{
    // Create simple colored icons using Qt's built-in shapes
    QSettings s;
    QColor fontColor = s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>();
    
    // Original icons
    m_cpuIcon = createColoredIcon(":/icons/cpu.svg", fontColor);
    m_memIcon = createColoredIcon(":/icons/memory.svg", fontColor);
    m_ramIcon = createColoredIcon(":/icons/ram.svg", fontColor);
    m_diskIcon = createColoredIcon(":/icons/disk.svg", fontColor);
    m_gpuIcon = createColoredIcon(":/icons/gpu.svg", fontColor);
    
    // New metric icons
    m_fpsIcon = createColoredIcon(":/icons/fps.svg", fontColor);
    m_netDownIcon = createColoredIcon(":/icons/download.svg", fontColor);
    m_netUpIcon = createColoredIcon(":/icons/upload.svg", fontColor);
    m_dailyDataIcon = createColoredIcon(":/icons/data.svg", fontColor);
    m_cpuTempIcon = createColoredIcon(":/icons/temp.svg", fontColor);
    m_gpuTempIcon = createColoredIcon(":/icons/temp.svg", fontColor);
    m_processesIcon = createColoredIcon(":/icons/processes.svg", fontColor);
    m_uptimeIcon = createColoredIcon(":/icons/uptime.svg", fontColor);
}chip symbol
    m_cpuIcon = createColoredIcon(":/icons/cpu.svg", fontColor);
    
    // Memory icon - RAM bars
    m_memIcon = createColoredIcon(":/icons/memory.svg", fontColor);
    
    // RAM icon - memory module
    m_ramIcon = createColoredIcon(":/icons/ram.svg", fontColor);
    
    // Disk icon - hard drive
    m_diskIcon = createColoredIcon(":/icons/disk.svg", fontColor);
    
    // GPU icon - graphics card
    m_gpuIcon = createColoredIcon(":/icons/gpu.svg", fontColor);
}

QPixmap OverlayWidget::createColoredIcon(const QString& iconPath, const QColor& color, const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.5));
    painter.setBrush(Qt::NoBrush);

    // Create simple geometric icons since we don't have SVG files
    if (iconPath.contains("cpu")) {
        // CPU - draw a simple chip
        painter.drawRect(2, 2, 12, 12);
        painter.drawRect(4, 4, 8, 8);
        // Pins
        painter.drawLine(0, 4, 2, 4);
        painter.drawLine(0, 8, 2, 8);
        painter.drawLine(0, 12, 2, 12);
        painter.drawLine(14, 4, 16, 4);
        painter.drawLine(14, 8, 16, 8);
        painter.drawLine(14, 12, 16, 12);
    } else if (iconPath.contains("memory") || iconPath.contains("ram")) {
        // Memory - draw RAM bars
        painter.drawRect(2, 2, 3, 12);
        painter.drawRect(6, 2, 3, 12);
        painter.drawRect(11, 2, 3, 12);
        // Notches
        painter.drawLine(2, 6, 5, 6);
        painter.drawLine(6, 6, 9, 6);
        painter.drawLine(11, 6, 14, 6);
    } else if (iconPath.contains("disk")) {
        // Disk - draw a hard drive
        painter.drawRect(2, 4, 12, 8);
        painter.drawEllipse(4, 6, 4, 4);
        painter.drawEllipse(6, 8, 2, 2);
    } else if (iconPath.contains("gpu")) {
        // GPU - draw a graphics card
        painter.drawRect(1, 4, 14, 8);
        painter.drawRect(3, 6, 10, 4);
        // Connector
        painter.drawLine(15, 6, 16, 6);
        painter.drawLine(15, 8, 16, 8);
        painter.drawLine(15, 10, 16, 10);
    } else if (iconPath.contains("fps")) {
        // FPS - draw a monitor/screen
        painter.drawRect(2, 3, 12, 9);
        painter.drawRect(3, 4, 10, 7);
        painter.drawLine(7, 12, 9, 12); // Stand
        painter.drawLine(6, 13, 10, 13); // Base
    } else if (iconPath.contains("download")) {
        // Download - arrow down
        painter.drawLine(8, 2, 8, 12);
        painter.drawLine(8, 12, 5, 9);
        painter.drawLine(8, 12, 11, 9);
        painter.drawLine(4, 14, 12, 14);
    } else if (iconPath.contains("upload")) {
        // Upload - arrow up
        painter.drawLine(8, 14, 8, 4);
        painter.drawLine(8, 4, 5, 7);
        painter.drawLine(8, 4, 11, 7);
        painter.drawLine(4, 2, 12, 2);
    } else if (iconPath.contains("data")) {
        // Data usage - pie chart
        painter.drawEllipse(2, 2, 12, 12);
        painter.drawLine(8, 8, 8, 2);
        painter.drawLine(8, 8, 14, 8);
        painter.drawArc(2, 2, 12, 12, 0, 90 * 16);
    } else if (iconPath.contains("temp")) {
        // Temperature - thermometer
        painter.drawRect(7, 2, 2, 10);
        painter.drawEllipse(5, 10, 6, 6);
        painter.drawLine(7, 4, 9, 4);
        painter.drawLine(7, 6, 9, 6);
        painter.drawLine(7, 8, 9, 8);
    } else if (iconPath.contains("processes")) {
        // Processes - multiple rectangles
        painter.drawRect(2, 2, 6, 4);
        painter.drawRect(4, 5, 6, 4);
        painter.drawRect(6, 8, 6, 4);
        painter.drawRect(8, 11, 6, 4);
    } else if (iconPath.contains("uptime")) {
        // Uptime - clock
        painter.drawEllipse(2, 2, 12, 12);
        painter.drawLine(8, 8, 8, 5); // Hour hand
        painter.drawLine(8, 8, 11, 8); // Minute hand
        painter.drawPoint(8, 8); // Center
    }

    return pixmap;
}

void OverlayWidget::createLayout()
{
    // Create the layout and container widgets ONCE
    QSettings s;
    QString orientation = s.value("appearance/layoutOrientation", "Vertical").toString();
    
    QBoxLayout* mainLayout;
    if (orientation == "Horizontal") {
        mainLayout = new QHBoxLayout(this);
        mainLayout->setSpacing(8);
    } else { // Default to Vertical
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(2);
    }
    mainLayout->setContentsMargins(5, 2, 5, 2);

    // Create horizontal layouts for each metric with icon + text
    auto createMetricLayout = [this, orientation](QLabel* label, const QPixmap& icon) -> QWidget* {
        QWidget* widget = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(5);
        
        QLabel* iconLabel = new QLabel(widget);
        iconLabel->setPixmap(icon);
        iconLabel->setFixedSize(16, 16);
        iconLabel->setScaledContents(true);
        
        layout->addWidget(iconLabel);
        layout->addWidget(label);
        
        // Only add stretch in horizontal orientation to prevent icons from spreading out
        if (orientation == "Horizontal") {
            layout->addStretch();
        }
        
        return widget;
    };

void OverlayWidget::createLayout()
{
    // Create the layout and container widgets ONCE
    QSettings s;
    QString orientation = s.value("appearance/layoutOrientation", "Vertical").toString();
    
    QBoxLayout* mainLayout;
    if (orientation == "Horizontal") {
        mainLayout = new QHBoxLayout(this);
        mainLayout->setSpacing(8);
    } else { // Default to Vertical
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(2);
    }
    mainLayout->setContentsMargins(5, 2, 5, 2);

    // Create horizontal layouts for each metric with icon + text
    auto createMetricLayout = [this, orientation](QLabel* label, const QPixmap& icon) -> QWidget* {
        QWidget* widget = new QWidget(this);
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(5);
        
        QLabel* iconLabel = new QLabel(widget);
        iconLabel->setPixmap(icon);
        iconLabel->setFixedSize(16, 16);
        iconLabel->setScaledContents(true);
        
        layout->addWidget(iconLabel);
        layout->addWidget(label);
        
        // Only add stretch in horizontal orientation to prevent icons from spreading out
        if (orientation == "Horizontal") {
            layout->addStretch();
        }
        
        return widget;
    };

    // Create container widgets for original metrics
    m_cpuWidget = createMetricLayout(m_cpuLabel, m_cpuIcon);
    m_memWidget = createMetricLayout(m_memLabel, m_memIcon);
    m_ramWidget = createMetricLayout(m_ramLabel, m_ramIcon);
    m_diskWidget = createMetricLayout(m_diskLabel, m_diskIcon);
    m_gpuWidget = createMetricLayout(m_gpuLabel, m_gpuIcon);
    
    // Create container widgets for new metrics
    m_fpsWidget = createMetricLayout(m_fpsLabel, m_fpsIcon);
    m_netDownWidget = createMetricLayout(m_netDownLabel, m_netDownIcon);
    m_netUpWidget = createMetricLayout(m_netUpLabel, m_netUpIcon);
    m_dailyDataWidget = createMetricLayout(m_dailyDataLabel, m_dailyDataIcon);
    m_cpuTempWidget = createMetricLayout(m_cpuTempLabel, m_cpuTempIcon);
    m_gpuTempWidget = createMetricLayout(m_gpuTempLabel, m_gpuTempIcon);
    m_processesWidget = createMetricLayout(m_processesLabel, m_processesIcon);
    m_uptimeWidget = createMetricLayout(m_uptimeLabel, m_uptimeIcon);
    
    // Store references to icon labels for later updates
    m_cpuIconLabel = m_cpuWidget->findChild<QLabel*>();
    m_memIconLabel = m_memWidget->findChild<QLabel*>();
    m_ramIconLabel = m_ramWidget->findChild<QLabel*>();
    m_diskIconLabel = m_diskWidget->findChild<QLabel*>();
    m_gpuIconLabel = m_gpuWidget->findChild<QLabel*>();
    m_fpsIconLabel = m_fpsWidget->findChild<QLabel*>();
    m_netDownIconLabel = m_netDownWidget->findChild<QLabel*>();
    m_netUpIconLabel = m_netUpWidget->findChild<QLabel*>();
    m_dailyDataIconLabel = m_dailyDataWidget->findChild<QLabel*>();
    m_cpuTempIconLabel = m_cpuTempWidget->findChild<QLabel*>();
    m_gpuTempIconLabel = m_gpuTempWidget->findChild<QLabel*>();
    m_processesIconLabel = m_processesWidget->findChild<QLabel*>();
    m_uptimeIconLabel = m_uptimeWidget->findChild<QLabel*>();

    // Add widgets to layout
    mainLayout->addWidget(m_cpuWidget);
    mainLayout->addWidget(m_memWidget);
    mainLayout->addWidget(m_ramWidget);
    mainLayout->addWidget(m_diskWidget);
    mainLayout->addWidget(m_gpuWidget);
    mainLayout->addWidget(m_fpsWidget);
    mainLayout->addWidget(m_netDownWidget);
    mainLayout->addWidget(m_netUpWidget);
    mainLayout->addWidget(m_dailyDataWidget);
    mainLayout->addWidget(m_cpuTempWidget);
    mainLayout->addWidget(m_gpuTempWidget);
    mainLayout->addWidget(m_processesWidget);
    mainLayout->addWidget(m_uptimeWidget);
}
}

void OverlayWidget::loadSettings()
{
    QSettings s;

    // Restore position
    move(s.value("window/pos", QPoint(100, 100)).toPoint());

    // Recreate icons with current color
    createIcons();
    
    // Update icon labels with new icons
    if (m_cpuIconLabel) m_cpuIconLabel->setPixmap(m_cpuIcon);
    if (m_memIconLabel) m_memIconLabel->setPixmap(m_memIcon);
    if (m_ramIconLabel) m_ramIconLabel->setPixmap(m_ramIcon);
    if (m_diskIconLabel) m_diskIconLabel->setPixmap(m_diskIcon);
    if (m_gpuIconLabel) m_gpuIconLabel->setPixmap(m_gpuIcon);
    if (m_fpsIconLabel) m_fpsIconLabel->setPixmap(m_fpsIcon);
    if (m_netDownIconLabel) m_netDownIconLabel->setPixmap(m_netDownIcon);
    if (m_netUpIconLabel) m_netUpIconLabel->setPixmap(m_netUpIcon);
    if (m_dailyDataIconLabel) m_dailyDataIconLabel->setPixmap(m_dailyDataIcon);
    if (m_cpuTempIconLabel) m_cpuTempIconLabel->setPixmap(m_cpuTempIcon);
    if (m_gpuTempIconLabel) m_gpuTempIconLabel->setPixmap(m_gpuTempIcon);
    if (m_processesIconLabel) m_processesIconLabel->setPixmap(m_processesIcon);
    if (m_uptimeIconLabel) m_uptimeIconLabel->setPixmap(m_uptimeIcon);

    // Apply appearance settings to labels
    int fontSize = s.value("appearance/fontSize", 11).toInt();
    QColor fontColor = s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>();
    QString labelStyle = QString("QLabel { color: %1; font-size: %2px; font-weight: bold; }")
                             .arg(fontColor.name(QColor::HexRgb), QString::number(fontSize));

    // Apply styles to all text labels
    QList<QLabel*> allLabels = {
        m_cpuLabel, m_memLabel, m_ramLabel, m_diskLabel, m_gpuLabel,
        m_fpsLabel, m_netDownLabel, m_netUpLabel, m_dailyDataLabel,
        m_cpuTempLabel, m_gpuTempLabel, m_processesLabel, m_uptimeLabel
    };
    
    for (auto* label : allLabels) {
        label->setStyleSheet(labelStyle);
    }

    // Apply shadow effects to all text labels
    for (auto* label : allLabels) {
        auto* effect = new QGraphicsDropShadowEffect();
        effect->setBlurRadius(8);
        effect->setColor(QColor(0, 0, 0, 220));
        effect->setOffset(0, 0);
        label->setGraphicsEffect(effect);
    }

    // Apply visibility settings
    if (m_cpuWidget) m_cpuWidget->setVisible(s.value("display/showCpu", true).toBool());
    if (m_memWidget) m_memWidget->setVisible(s.value("display/showMem", true).toBool());
    if (m_ramWidget) m_ramWidget->setVisible(s.value("display/showRam", true).toBool());
    if (m_diskWidget) m_diskWidget->setVisible(s.value("display/showDisk", true).toBool());
    if (m_gpuWidget) m_gpuWidget->setVisible(s.value("display/showGpu", true).toBool());
    if (m_fpsWidget) m_fpsWidget->setVisible(s.value("display/showFps", false).toBool());
    if (m_netDownWidget) m_netDownWidget->setVisible(s.value("display/showNetDown", false).toBool());
    if (m_netUpWidget) m_netUpWidget->setVisible(s.value("display/showNetUp", false).toBool());
    if (m_dailyDataWidget) m_dailyDataWidget->setVisible(s.value("display/showDailyData", false).toBool());
    if (m_cpuTempWidget) m_cpuTempWidget->setVisible(s.value("display/showCpuTemp", false).toBool());
    if (m_gpuTempWidget) m_gpuTempWidget->setVisible(s.value("display/showGpuTemp", false).toBool());
    if (m_processesWidget) m_processesWidget->setVisible(s.value("display/showProcesses", false).toBool());
    if (m_uptimeWidget) m_uptimeWidget->setVisible(s.value("display/showUptime", false).toBool());

    // Check if layout orientation has changed
    QString currentOrientation = s.value("appearance/layoutOrientation", "Vertical").toString();
    QBoxLayout* currentLayout = qobject_cast<QBoxLayout*>(layout());
    bool isHorizontal = qobject_cast<QHBoxLayout*>(currentLayout) != nullptr;
    bool shouldBeHorizontal = (currentOrientation == "Horizontal");
    
    if (isHorizontal != shouldBeHorizontal) {
        // Layout orientation changed, need to recreate
        updateLayoutOrientation();
    }

    // Behavior
    m_monitor->setUpdateInterval(s.value("behavior/updateInterval", 1000).toInt());

    adjustSize();
    update(); // Trigger a repaint
}

void OverlayWidget::updateLayoutOrientation()
{
    // Only recreate layout if orientation actually changed
    QSettings s;
    QString orientation = s.value("appearance/layoutOrientation", "Vertical").toString();
    
    // Remove widgets from current layout
    if (layout()) {
        layout()->removeWidget(m_cpuWidget);
        layout()->removeWidget(m_memWidget);
        layout()->removeWidget(m_ramWidget);
        layout()->removeWidget(m_diskWidget);
        layout()->removeWidget(m_gpuWidget);
        layout()->removeWidget(m_fpsWidget);
        layout()->removeWidget(m_netDownWidget);
        layout()->removeWidget(m_netUpWidget);
        layout()->removeWidget(m_dailyDataWidget);
        layout()->removeWidget(m_cpuTempWidget);
        layout()->removeWidget(m_gpuTempWidget);
        layout()->removeWidget(m_processesWidget);
        layout()->removeWidget(m_uptimeWidget);
        delete layout();
    }
    
    // Create new layout with correct orientation
    QBoxLayout* newLayout;
    if (orientation == "Horizontal") {
        newLayout = new QHBoxLayout(this);
        newLayout->setSpacing(8);
    } else { // Default to Vertical
        newLayout = new QVBoxLayout(this);
        newLayout->setSpacing(2);
    }
    newLayout->setContentsMargins(5, 2, 5, 2);

    // Add widgets back to new layout
    newLayout->addWidget(m_cpuWidget);
    newLayout->addWidget(m_memWidget);
    newLayout->addWidget(m_ramWidget);
    newLayout->addWidget(m_diskWidget);
    newLayout->addWidget(m_gpuWidget);
    newLayout->addWidget(m_fpsWidget);
    newLayout->addWidget(m_netDownWidget);
    newLayout->addWidget(m_netUpWidget);
    newLayout->addWidget(m_dailyDataWidget);
    newLayout->addWidget(m_cpuTempWidget);
    newLayout->addWidget(m_gpuTempWidget);
    newLayout->addWidget(m_processesWidget);
    newLayout->addWidget(m_uptimeWidget);
}

void OverlayWidget::applySettings()
{
    loadSettings();
}

void OverlayWidget::openSettingsDialog()
{
    m_monitor->stopUpdates(); // Stop updates to prevent interference with the dialog

    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &OverlayWidget::applySettings);
    dialog.exec();

    m_monitor->resumeUpdates(); // Resume updates after the dialog is closed
}

OverlayWidget::~OverlayWidget()
{
}

void OverlayWidget::updateStats(const SysInfo &info)
{
    // Original metrics
    m_cpuLabel->setText(QString("CPU: %1%").arg(info.cpuLoad, 0, 'f', 1));
    m_memLabel->setText(QString("MEM: %1%").arg(info.memUsage));
    m_ramLabel->setText(QString("RAM: %1/%2 MB").arg(info.totalRamMB - info.availRamMB).arg(info.totalRamMB));
    m_diskLabel->setText(QString("DSK: %1%").arg(info.diskLoad, 0, 'f', 1));
    m_gpuLabel->setText(QString("GPU: %1%").arg(info.gpuLoad, 0, 'f', 1));

    // New metrics
    if (info.fps > 0) {
        m_fpsLabel->setText(QString("FPS: %1").arg(info.fps, 0, 'f', 0));
    } else {
        m_fpsLabel->setText("FPS: --");
    }
    
    m_netDownLabel->setText(QString("↓: %1 MB/s").arg(info.networkDownloadSpeed, 0, 'f', 2));
    m_netUpLabel->setText(QString("↑: %1 MB/s").arg(info.networkUploadSpeed, 0, 'f', 2));
    m_dailyDataLabel->setText(QString("Daily: %1 MB").arg(info.dailyDataUsageMB));
    
    if (info.cpuTemp > 0) {
        m_cpuTempLabel->setText(QString("CPU°: %1°C").arg(info.cpuTemp, 0, 'f', 1));
    } else {
        m_cpuTempLabel->setText("CPU°: --");
    }
    
    if (info.gpuTemp > 0) {
        m_gpuTempLabel->setText(QString("GPU°: %1°C").arg(info.gpuTemp, 0, 'f', 1));
    } else {
        m_gpuTempLabel->setText("GPU°: --");
    }
    
    m_processesLabel->setText(QString("Proc: %1").arg(info.activeProcesses));
    
    // Format uptime nicely
    if (info.systemUptime < 1) {
        m_uptimeLabel->setText(QString("Up: %1m").arg(info.systemUptime * 60, 0, 'f', 0));
    } else if (info.systemUptime < 24) {
        m_uptimeLabel->setText(QString("Up: %1h").arg(info.systemUptime, 0, 'f', 1));
    } else {
        m_uptimeLabel->setText(QString("Up: %1d").arg(info.systemUptime / 24, 0, 'f', 1));
    }

#ifdef Q_OS_WIN
    // Periodically re-apply the HWND_TOPMOST flag
    if (auto hwnd = reinterpret_cast<HWND>(winId())) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
#endif
}

void OverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // Save window position when done dragging
    if (event->button() == Qt::LeftButton) {
        QSettings s;
        s.setValue("window/pos", pos());
    }
    QWidget::mouseReleaseEvent(event);
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QSettings s;
    QColor bgColor = s.value("appearance/backgroundColor", QColor(Qt::black)).value<QColor>();
    int opacity = s.value("appearance/backgroundOpacity", 120).toInt();
    bgColor.setAlpha(opacity);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 5.0, 5.0);
}

void OverlayWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);
    
    // Add icons to context menu
    QAction *settingsAction = contextMenu.addAction("Settings...");
    settingsAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(settingsAction, &QAction::triggered, this, &OverlayWidget::openSettingsDialog);
    
    contextMenu.addSeparator();
    
    QAction *closeAction = contextMenu.addAction("Close");
    closeAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    connect(closeAction, &QAction::triggered, qApp, &QApplication::quit);
    
    contextMenu.exec(event->globalPos());
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}