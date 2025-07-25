#include "sysinfomonitor.h"
#include <QDebug>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    // Initialize PDH query for CPU usage
    if (PdhOpenQuery(nullptr, 0, &m_cpuQuery) != ERROR_SUCCESS) {
        qWarning() << "Failed to open PDH Query.";
    }
    // Using the English counter path is more reliable across different system locales
    if (PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter) != ERROR_SUCCESS) {
        qWarning() << "Failed to add PDH Counter.";
    }
    if (PdhCollectQueryData(m_cpuQuery) != ERROR_SUCCESS) {
        qWarning() << "Failed to collect initial query data.";
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::updateStats);
    m_timer->start(1000); // Update every second

    // Trigger initial update
    updateStats();
}

SysInfoMonitor::~SysInfoMonitor()
{
    if (m_cpuQuery) {
        PdhCloseQuery(m_cpuQuery);
    }
}

void SysInfoMonitor::updateStats()
{
    SysInfo info;

    // Get CPU Load
    PDH_FMT_COUNTERVALUE counterVal;
    if (PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.cpuLoad = counterVal.doubleValue;
    } else {
        info.cpuLoad = 0.0;
    }

    // Get Memory Usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.memUsage = memInfo.dwMemoryLoad;
    } else {
        info.memUsage = 0;
    }

    emit statsUpdated(info);
}