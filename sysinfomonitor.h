#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <Pdh.h>
#include <PdhMsg.h>
#endif

struct SysInfo {
    double cpuLoad;
    DWORD memUsage; // Memory usage percentage
    qint64 totalRamMB;
    qint64 availRamMB;
    double diskLoad;
    double gpuLoad;
};

class SysInfoMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SysInfoMonitor(QObject *parent = nullptr);
    ~SysInfoMonitor();

signals:
    void statsUpdated(const SysInfo& info);

private slots:
    void updateStats();

private:
    QTimer* m_timer;
#ifdef Q_OS_WIN
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuTotalCounter;
    PDH_HQUERY m_diskQuery;
    PDH_HCOUNTER m_diskTotalCounter;
    PDH_HQUERY m_gpuQuery;
    QList<PDH_HCOUNTER> m_gpuCounters;
#endif
};

#endif // SYSINFOMONITOR_H