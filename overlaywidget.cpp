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
    m_cpuLabel = new QLabel("CPU: ...", this);
    m_memLabel = new QLabel("MEM: ...", this);
    m_ramLabel = new QLabel("RAM: ...", this);
    m_diskLabel = new QLabel("DSK: ...", this);
    m_gpuLabel = new QLabel("GPU: ...", this);
    
    // Initialize container widgets to nullptr
    m_cpuWidget = nullptr;
    m_memWidget = nullptr;
    m_ramWidget = nullptr;
    m_diskWidget = nullptr;
    m_gpuWidget = nullptr;
}

void OverlayWidget::createIcons()
{
    // Create simple colored icons using Qt's built-in shapes
    QSettings s;
    QColor fontColor = s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>();
    
    // CPU icon - processor/chip symbol
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
    // You can replace these with actual icon files later
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

    // Create container widgets
    m_cpuWidget = createMetricLayout(m_cpuLabel, m_cpuIcon);
    m_memWidget = createMetricLayout(m_memLabel, m_memIcon);
    m_ramWidget = createMetricLayout(m_ramLabel, m_ramIcon);
    m_diskWidget = createMetricLayout(m_diskLabel, m_diskIcon);
    m_gpuWidget = createMetricLayout(m_gpuLabel, m_gpuIcon);
    
    // Store references to icon labels for later updates
    m_cpuIconLabel = m_cpuWidget->findChild<QLabel*>();
    m_memIconLabel = m_memWidget->findChild<QLabel*>();
    m_ramIconLabel = m_ramWidget->findChild<QLabel*>();
    m_diskIconLabel = m_diskWidget->findChild<QLabel*>();
    m_gpuIconLabel = m_gpuWidget->findChild<QLabel*>();

    // Add widgets to layout
    mainLayout->addWidget(m_cpuWidget);
    mainLayout->addWidget(m_memWidget);
    mainLayout->addWidget(m_ramWidget);
    mainLayout->addWidget(m_diskWidget);
    mainLayout->addWidget(m_gpuWidget);
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

    // Apply appearance settings to labels
    int fontSize = s.value("appearance/fontSize", 11).toInt();
    QColor fontColor = s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>();
    QString labelStyle = QString("QLabel { color: %1; font-size: %2px; font-weight: bold; }")
                             .arg(fontColor.name(QColor::HexRgb), QString::number(fontSize));

    // Apply styles only to text labels, not icon labels
    m_cpuLabel->setStyleSheet(labelStyle);
    m_memLabel->setStyleSheet(labelStyle);
    m_ramLabel->setStyleSheet(labelStyle);
    m_diskLabel->setStyleSheet(labelStyle);
    m_gpuLabel->setStyleSheet(labelStyle);

    // Apply shadow effects only to text labels
    for (auto* label : {m_cpuLabel, m_memLabel, m_ramLabel, m_diskLabel, m_gpuLabel}) {
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
    m_cpuLabel->setText(QString("CPU: %1%").arg(info.cpuLoad, 0, 'f', 1));
    m_memLabel->setText(QString("MEM: %1%").arg(info.memUsage));
    m_ramLabel->setText(QString("RAM: %1/%2 MB").arg(info.totalRamMB - info.availRamMB).arg(info.totalRamMB));
    m_diskLabel->setText(QString("DSK: %1%").arg(info.diskLoad, 0, 'f', 1));
    m_gpuLabel->setText(QString("GPU: %1%").arg(info.gpuLoad, 0, 'f', 1));

#ifdef Q_OS_WIN
    // Periodically re-apply the HWND_TOPMOST flag. This is a workaround to keep the widget
    // on top of other topmost windows like the Start Menu or Task Manager. When another
    // topmost window appears, it can cover our widget. This call brings our widget
    // back to the top of the Z-order for all topmost windows without activating it.
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