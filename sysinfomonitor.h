#ifndef SYSINFOMONITOR_H
#define SYSINFOMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <Pdh.h>
#endif

struct SysInfo {
    double cpuLoad;
    int memUsage;
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
    PDH_HQUERY m_cpuQuery;
    PDH_HCOUNTER m_cpuTotalCounter;
};

#endif // SYSINFOMONITOR_H