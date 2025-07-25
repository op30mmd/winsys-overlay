#include "overlaywidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>

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

    // Style the labels for better visibility on any background
    const QString labelStyle = "QLabel { color: white; font-size: 14px; font-weight: bold; }";
    m_cpuLabel->setStyleSheet(labelStyle);
    m_memLabel->setStyleSheet(labelStyle);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_cpuLabel);
    layout->addWidget(m_memLabel);
    setLayout(layout);

    connect(m_monitor, &SysInfoMonitor::statsUpdated, this, &OverlayWidget::updateStats);

    resize(150, 50);
}

OverlayWidget::~OverlayWidget()
{
}

void OverlayWidget::updateStats(const SysInfo &info)
{
    m_cpuLabel->setText(QString("CPU: %1%").arg(info.cpuLoad, 0, 'f', 1));
    m_memLabel->setText(QString("MEM: %1%").arg(info.memUsage));

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

void OverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}