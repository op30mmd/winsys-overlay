#include "sysinfomonitor.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDate>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_tempReaderProcess = new QProcess(this);
    m_timer = new QTimer(this);

    // Initialize daily data tracking
    m_dailyDataBytes = 0;
    m_lastResetDate = QDate::currentDate();
    
    // Initialize network tracking for speed calculation
    m_lastNetworkBytes = {0, 0};
    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();

    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::poll);
    connect(m_tempReaderProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SysInfoMonitor::onTempReaderFinished);

    // Add error handling to see what happens with the process
    connect(m_tempReaderProcess, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "QProcess error:" << error;
    });

    initializeLegacyCounters();
    loadDailyDataUsage();
}

SysInfoMonitor::~SysInfoMonitor()
{
    stop();
    saveDailyDataUsage();
}

void SysInfoMonitor::start() {
    QSettings s;
    int interval = s.value("behavior/updateInterval", 1000).toInt();
    m_timer->start(interval);
}

void SysInfoMonitor::stop() {
    m_timer->stop();
    m_tempReaderProcess->kill();
    saveDailyDataUsage();
}

void SysInfoMonitor::poll() {
    // Update the stats that don't come from the helper process
    updateLegacyStats(m_sysInfo);

    // Start the helper process to get temperatures
    if (m_tempReaderProcess->state() == QProcess::NotRunning) {
        QString programPath = QCoreApplication::applicationDirPath() + QDir::separator() + "TempReader.exe";
        qDebug() << "Attempting to run TempReader from:" << programPath;
        m_tempReaderProcess->start(programPath);
    }
}

void SysInfoMonitor::onTempReaderFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QByteArray output = m_tempReaderProcess->readAllStandardOutput();
        QString outputStr(output.trimmed());
        qDebug() << "TempReader output:" << outputStr;

        // Parse the output: "CPU:XX.X,GPU:XX.X"
        QStringList parts = outputStr.split(',');
        if (parts.length() == 2) {
            QString cpuTempStr = parts[0].mid(4); // Remove "CPU:"
            QString gpuTempStr = parts[1].mid(4); // Remove "GPU:"
            
            bool cpuOk, gpuOk;
            double cpuTemp = cpuTempStr.toDouble(&cpuOk);
            double gpuTemp = gpuTempStr.toDouble(&gpuOk);
            
            m_sysInfo.cpuTemp = cpuOk ? cpuTemp : -1;
            m_sysInfo.gpuTemp = gpuOk ? gpuTemp : -1;
        } else {
            qWarning() << "Failed to parse TempReader output:" << outputStr;
            m_sysInfo.cpuTemp = -1;
            m_sysInfo.gpuTemp = -1;
        }
    } else {
        qWarning() << "TempReader.exe failed or crashed. Exit code:" << exitCode << "Status:" << exitStatus;
        qWarning() << "Stderr:" << m_tempReaderProcess->readAllStandardError();
        m_sysInfo.cpuTemp = -1;
        m_sysInfo.gpuTemp = -1;
    }

    // Emit the final, combined stats
    emit statsUpdated(m_sysInfo);
}

void SysInfoMonitor::loadDailyDataUsage()
{
    QSettings s;
    QDate savedDate = s.value("network/lastResetDate", QDate::currentDate()).toDate();
    
    // Reset if it's a new day
    if (savedDate != QDate::currentDate()) {
        m_dailyDataBytes = 0;
        m_lastResetDate = QDate::currentDate();
        saveDailyDataUsage();
    } else {
        m_dailyDataBytes = s.value("network/dailyDataBytes", 0).toLongLong();
    }
}

void SysInfoMonitor::saveDailyDataUsage()
{
    QSettings s;
    s.setValue("network/dailyDataBytes", m_dailyDataBytes);
    s.setValue("network/lastResetDate", m_lastResetDate);
}

// --- Restored Legacy PDH Code ---

void SysInfoMonitor::initializeLegacyCounters() {
    // Initialize all queries and counters
    PdhOpenQuery(nullptr, 0, &m_cpuQuery);
    PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter);
    PdhCollectQueryData(m_cpuQuery);

    PdhOpenQuery(nullptr, 0, &m_diskQuery);
    PdhAddEnglishCounter(m_diskQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskTotalCounter);
    PdhCollectQueryData(m_diskQuery);

    // GPU counters
    PdhOpenQuery(nullptr, 0, &m_gpuQuery);
    const wchar_t* gpuCounterPath = L"\\GPU Engine(*)\\Utilization Percentage";
    DWORD gpuBufferSize = 0;
    if (PdhExpandWildCardPathW(nullptr, gpuCounterPath, nullptr, &gpuBufferSize, 0) == PDH_MORE_DATA) {
        QVector<wchar_t> gpuPathBuffer(gpuBufferSize);
        if (PdhExpandWildCardPathW(nullptr, gpuCounterPath, gpuPathBuffer.data(), &gpuBufferSize, 0) == ERROR_SUCCESS) {
            for (const wchar_t* p = gpuPathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                PDH_HCOUNTER gpuCounter;
                if (PdhAddEnglishCounterW(m_gpuQuery, p, 0, &gpuCounter) == ERROR_SUCCESS) {
                    m_gpuCounters.append(gpuCounter);
                }
            }
        }
    }
    if (!m_gpuCounters.isEmpty()) {
        PdhCollectQueryData(m_gpuQuery);
    }

    // Network counters
    PdhOpenQuery(nullptr, 0, &m_networkQuery);
    const wchar_t* receivedPath = L"\\Network Interface(*)\\Bytes Received/sec";
    const wchar_t* sentPath = L"\\Network Interface(*)\\Bytes Sent/sec";
    
    auto addCounters = [&](const wchar_t* path, QList<PDH_HCOUNTER>& list) {
        DWORD bufferSize = 0;
        if (PdhExpandWildCardPathW(nullptr, path, nullptr, &bufferSize, 0) == PDH_MORE_DATA) {
            QVector<wchar_t> pathBuffer(bufferSize);
            if (PdhExpandWildCardPathW(nullptr, path, pathBuffer.data(), &bufferSize, 0) == ERROR_SUCCESS) {
                for (const wchar_t* p = pathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                    QString interfaceName = QString::fromWCharArray(p);
                    // Skip loopback and other non-physical interfaces
                    if (!interfaceName.contains("Loopback", Qt::CaseInsensitive) &&
                        !interfaceName.contains("Teredo", Qt::CaseInsensitive) &&
                        !interfaceName.contains("isatap", Qt::CaseInsensitive)) {
                        PDH_HCOUNTER counter;
                        if (PdhAddEnglishCounterW(m_networkQuery, p, 0, &counter) == ERROR_SUCCESS) {
                            list.append(counter);
                        }
                    }
                }
            }
        }
    };
    
    addCounters(receivedPath, m_networkBytesReceivedCounters);
    addCounters(sentPath, m_networkBytesSentCounters);
    
    if (!m_networkBytesReceivedCounters.isEmpty() || !m_networkBytesSentCounters.isEmpty()) {
        PdhCollectQueryData(m_networkQuery);
    }
}

void SysInfoMonitor::updateLegacyStats(SysInfo& info) {
    PDH_FMT_COUNTERVALUE counterVal;

    // CPU Load
    if (m_cpuQuery && PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.cpuLoad = counterVal.doubleValue;
    } else {
        info.cpuLoad = 0.0;
    }

    // Memory Usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.memUsage = memInfo.dwMemoryLoad;
        info.totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
        info.availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    } else {
        info.memUsage = 0;
        info.totalRamMB = 0;
        info.availRamMB = 0;
    }

    // Disk Load
    if (m_diskQuery && PdhCollectQueryData(m_diskQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_diskTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.diskLoad = counterVal.doubleValue;
    } else {
        info.diskLoad = 0.0;
    }

    // GPU Load
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
    } else {
        info.gpuLoad = 0.0;
    }

    // Network Speed and Daily Usage
    if (m_networkQuery && PdhCollectQueryData(m_networkQuery) == ERROR_SUCCESS) {
        auto getCounterValue = [&](QList<PDH_HCOUNTER>& counters) {
            double total = 0.0;
            for (PDH_HCOUNTER counter : counters) {
                if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
                    total += counterVal.doubleValue;
                }
            }
            return total;
        };
        
        double currentDownBytes = getCounterValue(m_networkBytesReceivedCounters);
        double currentUpBytes = getCounterValue(m_networkBytesSentCounters);
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        
        // Calculate speeds (bytes per second to MB per second)
        if (m_lastNetworkTime > 0) {
            double timeDiffSec = (currentTime - m_lastNetworkTime) / 1000.0;
            if (timeDiffSec > 0) {
                double downSpeedBps = (currentDownBytes - m_lastNetworkBytes.first) / timeDiffSec;
                double upSpeedBps = (currentUpBytes - m_lastNetworkBytes.second) / timeDiffSec;
                
                info.networkDownloadSpeed = qMax(0.0, downSpeedBps / (1024.0 * 1024.0));
                info.networkUploadSpeed = qMax(0.0, upSpeedBps / (1024.0 * 1024.0));
                
                // Update daily data usage
                m_dailyDataBytes += qMax(0.0, downSpeedBps + upSpeedBps) * timeDiffSec;
            }
        }
        
        m_lastNetworkBytes = {currentDownBytes, currentUpBytes};
        m_lastNetworkTime = currentTime;
        
        info.dailyDataUsageMB = m_dailyDataBytes / (1024.0 * 1024.0);
    } else {
        info.networkDownloadSpeed = 0.0;
        info.networkUploadSpeed = 0.0;
        info.dailyDataUsageMB = m_dailyDataBytes / (1024.0 * 1024.0);
    }

    // Process Count
    DWORD processIds[1024];
    DWORD bytesNeeded;
    if (EnumProcesses(processIds, sizeof(processIds), &bytesNeeded)) {
        info.activeProcesses = bytesNeeded / sizeof(DWORD);
    } else {
        info.activeProcesses = 0;
    }

    // System Uptime
    ULONGLONG uptimeMs = GetTickCount64();
    info.systemUptime = uptimeMs / (1000.0 * 60.0 * 60.0); // Convert to hours

    // FPS - placeholder (would need more complex implementation)
    info.fps = 0.0; // Not implemented yet
    
    // Check if we need to reset daily data (new day)
    if (QDate::currentDate() != m_lastResetDate) {
        m_dailyDataBytes = 0;
        m_lastResetDate = QDate::currentDate();
        saveDailyDataUsage();
    }
}
