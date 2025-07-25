#include "overlaywidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QApplication>

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

    m_cpuLabel = new QLabel("CPU: ...", this);
    m_memLabel = new QLabel("MEM: ...", this);
    m_ramLabel = new QLabel("RAM: ...", this);
    m_diskLabel = new QLabel("DSK: ...", this);
    m_gpuLabel = new QLabel("GPU: ...", this);

    // Style the labels for better visibility on any background
    const QString labelStyle = "QLabel { color: white; font-size: 11px; font-weight: bold; }";
    m_cpuLabel->setStyleSheet(labelStyle);
    m_memLabel->setStyleSheet(labelStyle);
    m_ramLabel->setStyleSheet(labelStyle);
    m_diskLabel->setStyleSheet(labelStyle);
    m_gpuLabel->setStyleSheet(labelStyle);

    // Add a shadow effect to all labels to improve readability on any background
    for (auto* label : findChildren<QLabel*>()) {
        auto* effect = new QGraphicsDropShadowEffect();
        // A centered, blurred, dark shadow creates a "glow" effect that acts as a soft outline
        effect->setBlurRadius(8);
        effect->setColor(QColor(0, 0, 0, 220));
        effect->setOffset(0, 0);
        label->setGraphicsEffect(effect);
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(0);
    layout->addWidget(m_cpuLabel);
    layout->addWidget(m_memLabel);
    layout->addWidget(m_ramLabel);
    layout->addWidget(m_diskLabel);
    layout->addWidget(m_gpuLabel);
    setLayout(layout);

    connect(m_monitor, &SysInfoMonitor::statsUpdated, this, &OverlayWidget::updateStats);

    // Adjust size to be more compact
    resize(120, 85);
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

void OverlayWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);
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