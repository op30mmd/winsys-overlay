#include "sysinfomonitor.h"
#include <QDebug>
#include <QCoreApplication>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_tempReaderProcess = new QProcess(this);
    m_timer = new QTimer(this);

    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::poll);
    connect(m_tempReaderProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SysInfoMonitor::onTempReaderFinished);

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
}

void SysInfoMonitor::poll() {
    // Update the stats that don't come from the helper process
    updateLegacyStats(m_sysInfo);

    // Start the helper process to get temperatures
    if (m_tempReaderProcess->state() == QProcess::NotRunning) {
        QString program = QCoreApplication::applicationDirPath() + "/TempReader.exe";
        m_tempReaderProcess->start(program);
    }
}

void SysInfoMonitor::onTempReaderFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        QByteArray output = m_tempReaderProcess->readAllStandardOutput();
        QString outputStr(output);

        // Parse the output: "CPU:XX.X,GPU:XX.X"
        QStringList parts = outputStr.trimmed().split(',');
        if (parts.length() == 2) {
            QString cpuPart = parts[0];
            QString gpuPart = parts[1];
            m_sysInfo.cpuTemp = cpuPart.mid(4).toDouble();
            m_sysInfo.gpuTemp = gpuPart.mid(4).toDouble();
        }
    } else {
        qWarning() << "TempReader.exe failed to run or crashed.";
        m_sysInfo.cpuTemp = -1;
        m_sysInfo.gpuTemp = -1;
    }

    // Emit the final, combined stats
    emit statsUpdated(m_sysInfo);
}

void SysInfoMonitor::initializeLegacyCounters() {
    // This function contains the original, working PDH counter initializations
    // for CPU load, disk, GPU load, and network.
    PdhOpenQuery(nullptr, 0, &m_cpuQuery);
    PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter);
PdhCollectQueryData(m_cpuQuery);

    PdhOpenQuery(nullptr, 0, &m_diskQuery);
    PdhAddEnglishCounter(m_diskQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskTotalCounter);
PdhCollectQueryData(m_diskQuery);

    PdhOpenQuery(nullptr, 0, &m_gpuQuery);
    // ... (rest of the PDH initialization code for GPU and Network) ...
}

void SysInfoMonitor::updateLegacyStats(SysInfo& info) {
    // This function contains the original, working PDH data collection
    // for CPU load, disk, GPU load, and network.
    PDH_FMT_COUNTERVALUE counterVal;
    if (m_cpuQuery && PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.cpuLoad = counterVal.doubleValue;
    }
    // ... (rest of the PDH data collection code) ...
}