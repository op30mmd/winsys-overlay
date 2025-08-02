#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QPair>

#ifdef Q_OS_WIN
#include <windows.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <iphlpapi.h>
#include <psapi.h>
#endif

struct SysInfo {
    double cpuLoad = 0.0;
    DWORD memUsage = 0;
    qint64 totalRamMB = 0;
    qint64 availRamMB = 0;
    double diskLoad = 0.0;
    double gpuLoad = 0.0;
    double fps = 0.0;
    double networkDownloadSpeed = 0.0;
    double networkUploadSpeed = 0.0;
    qint64 dailyDataUsageMB = 0;
    double cpuTemp = -1.0;
    double gpuTemp = -1.0;
    int activeProcesses = 0;
    double systemUptime = 0.0;
};

class SysInfoMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SysInfoMonitor(QObject *parent = nullptr);
    ~SysInfoMonitor();

    void start();
    void stop();

signals:
    void statsUpdated(const SysInfo& info);

private slots:
    void poll();
    void readTempData();
    void onTempReaderFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initializeLegacyCounters();
    void updateLegacyStats(SysInfo& info);
    void loadDailyDataUsage();
    void saveDailyDataUsage();
    void startTempReaderProcess();

    QTimer* m_timer;
    QProcess* m_tempReaderProcess;
    SysInfo m_sysInfo;

    // Daily data tracking
    qint64 m_dailyDataBytes;
    QDate m_lastResetDate;
    
    // Network speed calculation
    qint64 m_lastNetworkTime;

#ifdef Q_OS_WIN
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuTotalCounter;
    PDH_HQUERY m_diskQuery;
    PDH_HCOUNTER m_diskTotalCounter;
    PDH_HQUERY m_gpuQuery;
    QList<PDH_HCOUNTER> m_gpuCounters;
    PDH_HQUERY m_networkQuery;
    QList<PDH_HCOUNTER> m_networkBytesReceivedCounters;
    QList<PDH_HCOUNTER> m_networkBytesSentCounters;
#endif
};

#endif // SYSINFOMONITOR_H
