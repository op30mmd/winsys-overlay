#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QString>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <iphlpapi.h>
#include <psapi.h>
#endif

struct SysInfo {
    double cpuLoad;
    DWORD memUsage;
    qint64 totalRamMB;
    qint64 availRamMB;
    double diskLoad;
    double gpuLoad;
    double fps;
    double networkDownloadSpeed;
    double networkUploadSpeed;
    qint64 dailyDataUsageMB;
    double cpuTemp;
    double gpuTemp;
    int activeProcesses;
    double systemUptime;
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
    void onTempReaderFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initializeLegacyCounters();
    void updateLegacyStats(SysInfo& info);

    QTimer* m_timer;
    QProcess* m_tempReaderProcess;
    SysInfo m_sysInfo;

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