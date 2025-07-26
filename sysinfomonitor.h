#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QDateTime>
#include <QThread>
#include "HardwareMonitor.h"

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

    void start();
    void stop();

signals:
    void statsUpdated(const SysInfo& info);

private slots:
    void poll();

private:
    void initialize();

    QThread* m_thread;
    QTimer* m_timer;
    HardwareMonitor* m_hardwareMonitor;
    SysInfo m_sysInfo;
};

#endif // SYSINFOMONITOR_H
