#include "sysinfomonitor.h"
#include <QDebug>
#include <QVector>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_cpuQuery = nullptr;
    m_diskQuery = nullptr;
    m_gpuQuery = nullptr;

    // Initialize PDH query for CPU usage
    if (PdhOpenQuery(nullptr, 0, &m_cpuQuery) == ERROR_SUCCESS) {
        // Using the English counter path is more reliable across different system locales
        if (PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter) != ERROR_SUCCESS) {
            qWarning() << "Failed to add CPU PDH Counter.";
        }
        PdhCollectQueryData(m_cpuQuery); // Initial collection
    } else {
        qWarning() << "Failed to open CPU PDH Query.";
    }

    // Initialize PDH query for Disk usage
    if (PdhOpenQuery(nullptr, 0, &m_diskQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounter(m_diskQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskTotalCounter) != ERROR_SUCCESS) {
            qWarning() << "Failed to add Disk PDH Counter.";
        }
        PdhCollectQueryData(m_diskQuery); // Initial collection
    } else {
        qWarning() << "Failed to open Disk PDH Query.";
    }

    // Initialize PDH query for GPU usage
    if (PdhOpenQuery(nullptr, 0, &m_gpuQuery) == ERROR_SUCCESS) {
        const wchar_t* counterPath = L"\\GPU Engine(*)\\Utilization Percentage";
        DWORD bufferSize = 0;
        // Get the required buffer size by calling with a null buffer
        if (PdhExpandWildCardPathW(nullptr, counterPath, nullptr, &bufferSize, 0) == PDH_MORE_DATA) {
            QVector<wchar_t> pathBuffer(bufferSize);
            if (PdhExpandWildCardPathW(nullptr, counterPath, pathBuffer.data(), &bufferSize, 0) == ERROR_SUCCESS) {
                // The buffer contains a list of null-terminated strings, with a final double-null terminator.
                for (const wchar_t* p = pathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                    PDH_HCOUNTER gpuCounter;
                    if (PdhAddEnglishCounterW(m_gpuQuery, p, 0, &gpuCounter) == ERROR_SUCCESS) {
                        m_gpuCounters.append(gpuCounter);
                    }
                }
            }
        }
        if (!m_gpuCounters.isEmpty()) {
            PdhCollectQueryData(m_gpuQuery); // Initial collection
        } else {
            qWarning() << "Could not find any GPU counters. GPU monitoring will be disabled.";
            PdhCloseQuery(m_gpuQuery);
            m_gpuQuery = nullptr;
        }
    } else {
        qWarning() << "Failed to open GPU PDH Query.";
        m_gpuQuery = nullptr;
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::updateStats);
    m_timer->start(1000); // Update every second

    // Trigger initial update
    updateStats();
}

void SysInfoMonitor::setUpdateInterval(int msec)
{
    if (m_timer->interval() != msec) {
        m_timer->start(msec);
    }
}

SysInfoMonitor::~SysInfoMonitor()
{
    if (m_cpuQuery) {
        PdhCloseQuery(m_cpuQuery);
    }
    if (m_diskQuery) {
        PdhCloseQuery(m_diskQuery);
    }
    if (m_gpuQuery) {
        PdhCloseQuery(m_gpuQuery);
    }
}

void SysInfoMonitor::updateStats()
{
    SysInfo info = {0}; // Initialize all members to 0/0.0

    // Get CPU Load
    PDH_FMT_COUNTERVALUE counterVal;
    if (m_cpuQuery && PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
            info.cpuLoad = counterVal.doubleValue;
    }

    // Get Memory Usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.memUsage = memInfo.dwMemoryLoad;
        info.totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
        info.availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    }

    // Get Disk Load
    if (m_diskQuery && PdhCollectQueryData(m_diskQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_diskTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS)
    {
        info.diskLoad = counterVal.doubleValue;
    }

    // Get GPU Load (max utilization across all engines)
    if (m_gpuQuery && !m_gpuCounters.isEmpty() && PdhCollectQueryData(m_gpuQuery) == ERROR_SUCCESS) {
        double maxGpuLoad = 0.0;
        for (PDH_HCOUNTER gpuCounter : m_gpuCounters) {
            if (PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
                if (counterVal.doubleValue > maxGpuLoad) {
                    maxGpuLoad = counterVal.doubleValue;
                }
            }
        }
        info.gpuLoad = maxGpuLoad;
    }

    emit statsUpdated(info);
}