#include "sysinfomonitor.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDate>
#include <QFileInfo>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_tempReaderProcess = new QProcess(this);
    m_timer = new QTimer(this);

    m_dailyDataBytes = 0;
    m_lastResetDate = QDate::currentDate();

    m_lastNetworkBytes = {0, 0};
    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();

    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::poll);
    connect(m_tempReaderProcess, &QProcess::readyReadStandardOutput, this, &SysInfoMonitor::readTempData);
    connect(m_tempReaderProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SysInfoMonitor::onTempReaderFinished);
    connect(m_tempReaderProcess, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "QProcess error:" << error;
    });

    initializeLegacyCounters();
    loadDailyDataUsage();
    startTempReaderProcess();
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
    if (m_tempReaderProcess->state() == QProcess::Running) {
        m_tempReaderProcess->write("exit\n");
        m_tempReaderProcess->waitForFinished(1000);
        m_tempReaderProcess->kill();
    }
    saveDailyDataUsage();
}

void SysInfoMonitor::startTempReaderProcess() {
    if (m_tempReaderProcess->state() != QProcess::NotRunning) {
        return;
    }
    QString programPath = QCoreApplication::applicationDirPath() + QDir::separator() + "TempReader.exe";
    QFileInfo tempReaderInfo(programPath);

    if (!tempReaderInfo.exists()) {
        qWarning() << "TempReader.exe not found at:" << programPath;
        m_sysInfo.cpuTemp = -1;
        m_sysInfo.gpuTemp = -1;
        return;
    }
    m_tempReaderProcess->start(programPath);
}


void SysInfoMonitor::poll() {
    updateLegacyStats(m_sysInfo);

    if (m_tempReaderProcess->state() == QProcess::Running) {
        m_tempReaderProcess->write("update\n");
    } else {
        startTempReaderProcess();
    }

    emit statsUpdated(m_sysInfo);
}

void SysInfoMonitor::readTempData() {
    while (m_tempReaderProcess->canReadLine()) {
        QByteArray output = m_tempReaderProcess->readLine();
        QString outputStr(output.trimmed());

        if (outputStr.contains("CPU:") && outputStr.contains("GPU:")) {
            QStringList parts = outputStr.split(',');
            if (parts.length() == 2) {
                m_sysInfo.cpuTemp = parts[0].mid(4).toDouble();
                m_sysInfo.gpuTemp = parts[1].mid(4).toDouble();
            }
        }
    }
}

void SysInfoMonitor::onTempReaderFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qWarning() << "TempReader.exe finished unexpectedly. Exit code:" << exitCode << "Status:" << exitStatus;
    m_sysInfo.cpuTemp = -1;
    m_sysInfo.gpuTemp = -1;
}

void SysInfoMonitor::loadDailyDataUsage()
{
    QSettings s;
    QDate savedDate = s.value("network/lastResetDate", QDate::currentDate()).toDate();

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

void SysInfoMonitor::initializeLegacyCounters() {
    PdhOpenQuery(nullptr, 0, &m_cpuQuery);
    PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter);
    PdhCollectQueryData(m_cpuQuery);

    PdhOpenQuery(nullptr, 0, &m_diskQuery);
    PdhAddEnglishCounter(m_diskQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskTotalCounter);
    PdhCollectQueryData(m_diskQuery);

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

    if (m_cpuQuery && PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.cpuLoad = counterVal.doubleValue;
    } else {
        info.cpuLoad = 0.0;
    }

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

    if (m_diskQuery && PdhCollectQueryData(m_diskQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_diskTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.diskLoad = counterVal.doubleValue;
    } else {
        info.diskLoad = 0.0;
    }

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
        
        if (m_lastNetworkTime > 0) {
            double timeDiffSec = (currentTime - m_lastNetworkTime) / 1000.0;
            if (timeDiffSec > 0) {
                double downSpeedBps = (currentDownBytes - m_lastNetworkBytes.first) / timeDiffSec;
                double upSpeedBps = (currentUpBytes - m_lastNetworkBytes.second) / timeDiffSec;
                
                info.networkDownloadSpeed = qMax(0.0, downSpeedBps / (1024.0 * 1024.0));
                info.networkUploadSpeed = qMax(0.0, upSpeedBps / (1024.0 * 1024.0));
                
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

    DWORD processIds[1024];
    DWORD bytesNeeded;
    if (EnumProcesses(processIds, sizeof(processIds), &bytesNeeded)) {
        info.activeProcesses = bytesNeeded / sizeof(DWORD);
    } else {
        info.activeProcesses = 0;
    }

    ULONGLONG uptimeMs = GetTickCount64();
    info.systemUptime = uptimeMs / (1000.0 * 60.0 * 60.0);

    info.fps = 0.0;
    
    if (QDate::currentDate() != m_lastResetDate) {
        m_dailyDataBytes = 0;
        m_lastResetDate = QDate::currentDate();
        saveDailyDataUsage();
    }
}
