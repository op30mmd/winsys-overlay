
#include "sysinfomonitor.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_tempReaderProcess = new QProcess(this);
    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::poll);
    connect(m_tempReaderProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SysInfoMonitor::onTempReaderFinished);

    // Add error handling to see what happens with the process
    connect(m_tempReaderProcess, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "QProcess error:" << error;
    });

    initializeLegacyCounters();
}

SysInfoMonitor::~SysInfoMonitor()
{
    stop();
}

void SysInfoMonitor::start() {
    m_timer->start(1000); // Poll every second
}

void SysInfoMonitor::stop() {
    m_timer->stop();
    m_tempReaderProcess->kill();
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
            m_sysInfo.cpuTemp = parts[0].mid(4).toDouble();
            m_sysInfo.gpuTemp = parts[1].mid(4).toDouble();
        } else {
            qWarning() << "Failed to parse TempReader output.";
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

// --- Restored Legacy PDH Code ---

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
                    if (wcsstr(p, L"Loopback") == nullptr) {
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
    }

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.memUsage = memInfo.dwMemoryLoad;
        info.totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
        info.availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    }

    if (m_diskQuery && PdhCollectQueryData(m_diskQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_diskTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.diskLoad = counterVal.doubleValue;
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
        info.networkDownloadSpeed = getCounterValue(m_networkBytesReceivedCounters) / (1024.0 * 1024.0);
        info.networkUploadSpeed = getCounterValue(m_networkBytesSentCounters) / (1024.0 * 1024.0);
    }

    DWORD processIds[1024];
    DWORD bytesNeeded;
    if (EnumProcesses(processIds, sizeof(processIds), &bytesNeeded)) {
        info.activeProcesses = bytesNeeded / sizeof(DWORD);
    }

    ULONGLONG uptimeMs = GetTickCount64();
    info.systemUptime = uptimeMs / (1000.0 * 60.0 * 60.0);
}
