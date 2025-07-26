#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
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
    DWORD memUsage; // Memory usage percentage
    qint64 totalRamMB;
    qint64 availRamMB;
    double diskLoad;
    double gpuLoad;
    
    // New metrics
    double fps;                    // Current FPS (estimated from active window)
    double networkDownloadSpeed;   // Current download speed in MB/s
    double networkUploadSpeed;     // Current upload speed in MB/s
    qint64 dailyDataUsageMB;      // Daily internet usage in MB
    double cpuTemp;               // CPU temperature in Celsius
    double gpuTemp;               // GPU temperature in Celsius
    int activeProcesses;          // Number of active processes
    double systemUptime;          // System uptime in hours
};

class SysInfoMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SysInfoMonitor(QObject *parent = nullptr);
    ~SysInfoMonitor();
    void setUpdateInterval(int msec);
    void stopUpdates();
    void resumeUpdates();

signals:
    void statsUpdated(const SysInfo& info);

private slots:
    void updateStats();

private:
    void initializeNetworkCounters();
    void updateNetworkStats(SysInfo& info);
    void updateFPS(SysInfo& info);
    void updateTemperatures(SysInfo& info);
    void updateSystemInfo(SysInfo& info);
    void saveDailyUsage();
    void loadDailyUsage();
    qint64 getCurrentDayKey();

    QTimer* m_timer;
    
    // Network tracking
    qint64 m_lastBytesReceived;
    qint64 m_lastBytesSent;
    QDateTime m_lastNetworkUpdate;
    qint64 m_dailyBytesReceived;
    qint64 m_dailyBytesSent;
    qint64 m_currentDayKey;
    
    // FPS tracking
    QDateTime m_lastFpsUpdate;
    int m_frameCount;
    double m_currentFps;
    
#ifdef Q_OS_WIN
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuTotalCounter;
    PDH_HQUERY m_diskQuery;
    PDH_HCOUNTER m_diskTotalCounter;
    PDH_HQUERY m_gpuQuery;
    QList<PDH_HCOUNTER> m_gpuCounters;
    
    // Network counters
    PDH_HQUERY m_networkQuery;
    QList<PDH_HCOUNTER> m_networkBytesReceivedCounters;
    QList<PDH_HCOUNTER> m_networkBytesSentCounters;
    
    // Temperature counters (if available)
    PDH_HQUERY m_tempQuery;
    QList<PDH_HCOUNTER> m_cpuTempCounters;
    QList<PDH_HCOUNTER> m_gpuTempCounters;

    // Daily data usage
    qint64 m_dailyBytesReceived;
    qint64 m_dailyBytesSent;
    qint64 m_currentDayKey;
#endif
};

#endif // SYSINFOMONITOR_H