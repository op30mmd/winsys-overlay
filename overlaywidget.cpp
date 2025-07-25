#include "overlaywidget.h"
#include "settingsdialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QApplication>
#include <QSettings>
#include <QPainter>

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
}

void OverlayWidget::loadSettings()
{
    QSettings s;

    // Restore position
    move(s.value("window/pos", QPoint(100, 100)).toPoint());

    // Apply appearance settings to labels
    int fontSize = s.value("appearance/fontSize", 11).toInt();
    QColor fontColor = s.value("appearance/fontColor", QColor(Qt::white)).value<QColor>();
    QString labelStyle = QString("QLabel { color: %1; font-size: %2px; font-weight: bold; }")
                             .arg(fontColor.name(QColor::HexRgb), QString::number(fontSize));

    for (auto* label : findChildren<QLabel*>()) {
        label->setStyleSheet(labelStyle);

        auto* effect = new QGraphicsDropShadowEffect();
        effect->setBlurRadius(8);
        effect->setColor(QColor(0, 0, 0, 220));
        effect->setOffset(0, 0);
        label->setGraphicsEffect(effect);
    }

    // Delete existing layout to prepare for a new one.
    // This is safe, as widgets are reparented to this widget.
    if (layout()) {
        delete layout();
    }

    // Apply new layout based on settings
    QString orientation = s.value("appearance/layoutOrientation", "Vertical").toString();
    QBoxLayout* newLayout;
    if (orientation == "Horizontal") {
        newLayout = new QHBoxLayout();
        newLayout->setSpacing(5);
    } else { // Default to Vertical
        newLayout = new QVBoxLayout();
        newLayout->setSpacing(0);
    }
    newLayout->setContentsMargins(5, 2, 5, 2);
    newLayout->addWidget(m_cpuLabel);
    newLayout->addWidget(m_memLabel);
    newLayout->addWidget(m_ramLabel);
    newLayout->addWidget(m_diskLabel);
    newLayout->addWidget(m_gpuLabel);
    setLayout(newLayout);

    // Visibility
    m_cpuLabel->setVisible(s.value("display/showCpu", true).toBool());
    m_memLabel->setVisible(s.value("display/showMem", true).toBool());
    m_ramLabel->setVisible(s.value("display/showRam", true).toBool());
    m_diskLabel->setVisible(s.value("display/showDisk", true).toBool());
    m_gpuLabel->setVisible(s.value("display/showGpu", true).toBool());

    // Behavior
    m_monitor->setUpdateInterval(s.value("behavior/updateInterval", 1000).toInt());

    adjustSize();
    update(); // Trigger a repaint
}

void OverlayWidget::applySettings()
{
    loadSettings();
}

void OverlayWidget::openSettingsDialog()
{
    SettingsDialog dialog(this);
    connect(&dialog, &SettingsDialog::settingsApplied, this, &OverlayWidget::applySettings);
    dialog.exec();
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
    contextMenu.addAction("Settings...", this, &OverlayWidget::openSettingsDialog);
    contextMenu.addSeparator();
    QAction *closeAction = contextMenu.addAction("Close");
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