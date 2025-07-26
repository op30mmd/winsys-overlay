#include "sysinfomonitor.h"
#include <QDebug>
#include <QVector>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

#include <WbemIdl.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

    m_cpuQuery = nullptr;
    m_diskQuery = nullptr;
    m_gpuQuery = nullptr;
    m_networkQuery = nullptr;
    m_tempQuery = nullptr;
    
    // Initialize network tracking
    m_lastBytesReceived = 0;
    m_lastBytesSent = 0;
    m_lastNetworkUpdate = QDateTime::currentDateTime();
    m_currentDayKey = getCurrentDayKey();
    loadDailyUsage();

    // Initialize PDH query for CPU usage
    if (PdhOpenQuery(nullptr, 0, &m_cpuQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounter(m_cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpuTotalCounter) != ERROR_SUCCESS) {
            qWarning() << "Failed to add CPU PDH Counter.";
        }
        PdhCollectQueryData(m_cpuQuery);
    } else {
        qWarning() << "Failed to open CPU PDH Query.";
    }

    // Initialize PDH query for Disk usage
    if (PdhOpenQuery(nullptr, 0, &m_diskQuery) == ERROR_SUCCESS) {
        if (PdhAddEnglishCounter(m_diskQuery, L"\\PhysicalDisk(_Total)\\% Disk Time", 0, &m_diskTotalCounter) != ERROR_SUCCESS) {
            qWarning() << "Failed to add Disk PDH Counter.";
        }
        PdhCollectQueryData(m_diskQuery);
    } else {
        qWarning() << "Failed to open Disk PDH Query.";
    }

    // Initialize PDH query for GPU usage
    if (PdhOpenQuery(nullptr, 0, &m_gpuQuery) == ERROR_SUCCESS) {
        const wchar_t* counterPath = L"\\GPU Engine(*)\\Utilization Percentage";
        DWORD bufferSize = 0;
        if (PdhExpandWildCardPathW(nullptr, counterPath, nullptr, &bufferSize, 0) == PDH_MORE_DATA) {
            QVector<wchar_t> pathBuffer(bufferSize);
            if (PdhExpandWildCardPathW(nullptr, counterPath, pathBuffer.data(), &bufferSize, 0) == ERROR_SUCCESS) {
                for (const wchar_t* p = pathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                    PDH_HCOUNTER gpuCounter;
                    if (PdhAddEnglishCounterW(m_gpuQuery, p, 0, &gpuCounter) == ERROR_SUCCESS) {
                        m_gpuCounters.append(gpuCounter);
                    }
                }
            }
        }
        if (!m_gpuCounters.isEmpty()) {
            PdhCollectQueryData(m_gpuQuery);
        } else {
            qWarning() << "Could not find any GPU counters. GPU monitoring will be disabled.";
            PdhCloseQuery(m_gpuQuery);
            m_gpuQuery = nullptr;
        }
    } else {
        qWarning() << "Failed to open GPU PDH Query.";
        m_gpuQuery = nullptr;
    }

    // Initialize network counters
    initializeNetworkCounters();

    // Initialize temperature monitoring
    if (PdhOpenQuery(nullptr, 0, &m_tempQuery) == ERROR_SUCCESS) {
        // Try to add CPU temperature counters (may not be available on all systems)
        const wchar_t* cpuTempPath = L"\\Thermal Zone Information(*)\\Temperature";
        DWORD tempBufferSize = 0;
        if (PdhExpandWildCardPathW(nullptr, cpuTempPath, nullptr, &tempBufferSize, 0) == PDH_MORE_DATA) {
            QVector<wchar_t> tempPathBuffer(tempBufferSize);
            if (PdhExpandWildCardPathW(nullptr, cpuTempPath, tempPathBuffer.data(), &tempBufferSize, 0) == ERROR_SUCCESS) {
                for (const wchar_t* p = tempPathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                    PDH_HCOUNTER tempCounter;
                    if (PdhAddEnglishCounterW(m_tempQuery, p, 0, &tempCounter) == ERROR_SUCCESS) {
                        m_cpuTempCounters.append(tempCounter);
                    }
                }
            }
        }
        
        if (!m_cpuTempCounters.isEmpty()) {
            PdhCollectQueryData(m_tempQuery);
        } else {
            qDebug() << "Temperature counters not available on this system.";
        }
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::updateStats);
    m_timer->start(1000);

    updateStats();
}

void SysInfoMonitor::initializeNetworkCounters()
{
    if (PdhOpenQuery(nullptr, 0, &m_networkQuery) == ERROR_SUCCESS) {
        const wchar_t* receivedPath = L"\Network Interface(*)\Bytes Received/sec";
        const wchar_t* sentPath = L"\Network Interface(*)\Bytes Sent/sec";

        auto addCounters = [&](const wchar_t* path, QList<PDH_HCOUNTER>& counterList) {
            DWORD bufferSize = 0;
            if (PdhExpandWildCardPathW(nullptr, path, nullptr, &bufferSize, 0) == PDH_MORE_DATA) {
                QVector<wchar_t> pathBuffer(bufferSize);
                if (PdhExpandWildCardPathW(nullptr, path, pathBuffer.data(), &bufferSize, 0) == ERROR_SUCCESS) {
                    for (const wchar_t* p = pathBuffer.data(); *p != L'\0'; p += wcslen(p) + 1) {
                        QString counterName = QString::fromWCharArray(p);
                        if (counterName.contains("Loopback") || counterName.contains("isatap")) continue;
                        PDH_HCOUNTER counter;
                        if (PdhAddEnglishCounterW(m_networkQuery, p, 0, &counter) == ERROR_SUCCESS) {
                            counterList.append(counter);
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
}

qint64 SysInfoMonitor::getCurrentDayKey()
{
    QDate today = QDate::currentDate();
    return today.year() * 10000 + today.month() * 100 + today.day();
}

void SysInfoMonitor::loadDailyUsage()
{
    QSettings s;
    qint64 todayKey = getCurrentDayKey();
    
    if (m_currentDayKey != todayKey) {
        // New day, reset counters
        m_dailyBytesReceived = 0;
        m_dailyBytesSent = 0;
        m_currentDayKey = todayKey;
    } else {
        // Same day, load existing usage
        m_dailyBytesReceived = s.value(QString("network/daily_received_%1").arg(todayKey), 0).toLongLong();
        m_dailyBytesSent = s.value(QString("network/daily_sent_%1").arg(todayKey), 0).toLongLong();
    }
}

void SysInfoMonitor::saveDailyUsage()
{
    QSettings s;
    qint64 todayKey = getCurrentDayKey();
    s.setValue(QString("network/daily_received_%1").arg(todayKey), m_dailyBytesReceived);
    s.setValue(QString("network/daily_sent_%1").arg(todayKey), m_dailyBytesSent);
    
    // Clean up old data (keep only last 30 days)
    QStringList keys = s.allKeys();
    QDate cutoffDate = QDate::currentDate().addDays(-30);
    qint64 cutoffKey = cutoffDate.year() * 10000 + cutoffDate.month() * 100 + cutoffDate.day();
    
    for (const QString& key : keys) {
        if (key.startsWith("network/daily_")) {
            QStringList parts = key.split('_');
            if (parts.size() >= 3) {
                qint64 keyDate = parts[2].toLongLong();
                if (keyDate < cutoffKey) {
                    s.remove(key);
                }
            }
        }
    }
}

void SysInfoMonitor::setUpdateInterval(int msec)
{
    if (m_timer->interval() != msec) {
        m_timer->start(msec);
    }
}

void SysInfoMonitor::stopUpdates()
{
    m_timer->stop();
}

void SysInfoMonitor::resumeUpdates()
{
    m_timer->start();
}

SysInfoMonitor::~SysInfoMonitor()
{
    saveDailyUsage();
    
    if (m_cpuQuery) PdhCloseQuery(m_cpuQuery);
    if (m_diskQuery) PdhCloseQuery(m_diskQuery);
    if (m_gpuQuery) PdhCloseQuery(m_gpuQuery);
    if (m_networkQuery) PdhCloseQuery(m_networkQuery);
    if (m_tempQuery) PdhCloseQuery(m_tempQuery);
}

void SysInfoMonitor::updateStats()
{
    SysInfo info = {0};

    // Get CPU Load
    PDH_FMT_COUNTERVALUE counterVal;
    if (m_cpuQuery && PdhCollectQueryData(m_cpuQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_cpuTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.cpuLoad = counterVal.doubleValue;
    }

    // Get Memory Usage
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        info.memUsage = memInfo.dwMemoryLoad;
        info.totalRamMB = memInfo.ullTotalPhys / (1024 * 1024);
        info.availRamMB = memInfo.ullAvailPhys / (1024 * 1024);
    }

    // Get Disk Load
    if (m_diskQuery && PdhCollectQueryData(m_diskQuery) == ERROR_SUCCESS &&
        PdhGetFormattedCounterValue(m_diskTotalCounter, PDH_FMT_DOUBLE, nullptr, &counterVal) == ERROR_SUCCESS) {
        info.diskLoad = counterVal.doubleValue;
    }

    // Get GPU Load
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

    // Update new metrics
    updateNetworkStats(info);
    updateFPS(info);
    updateTemperatures(info);
    updateSystemInfo(info);

    emit statsUpdated(info);
}

void SysInfoMonitor::updateNetworkStats(SysInfo& info)
{
    if (!m_networkQuery) return;

    if (PdhCollectQueryData(m_networkQuery) == ERROR_SUCCESS) {
        auto getCounterValue = [&](QList<PDH_HCOUNTER>& counters) {
            double total = 0.0;
            PDH_FMT_COUNTERVALUE value;
            for (PDH_HCOUNTER counter : counters) {
                if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) == ERROR_SUCCESS) {
                    total += value.doubleValue;
                }
            }
            return total;
        };

        info.networkDownloadSpeed = getCounterValue(m_networkBytesReceivedCounters) / (1024.0 * 1024.0);
        info.networkUploadSpeed = getCounterValue(m_networkBytesSentCounters) / (1024.0 * 1024.0);
    }

    qint64 currentDayKey = getCurrentDayKey();
    if (m_currentDayKey != currentDayKey) {
        m_dailyBytesReceived = 0;
        m_dailyBytesSent = 0;
        m_currentDayKey = currentDayKey;
    }

    MIB_IFTABLE* ifTable = nullptr;
    DWORD bufLen = 0;
    if (GetIfTable(ifTable, &bufLen, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        ifTable = (MIB_IFTABLE*)new char[bufLen];
        if (GetIfTable(ifTable, &bufLen, FALSE) == NO_ERROR) {
            qint64 totalReceived = 0;
            qint64 totalSent = 0;
            for (DWORD i = 0; i < ifTable->dwNumEntries; ++i) {
                totalReceived += ifTable->table[i].dwInOctets;
                totalSent += ifTable->table[i].dwOutOctets;
            }
            m_dailyBytesReceived = totalReceived;
            m_dailyBytesSent = totalSent;
        }
        delete[] ifTable;
    }

    info.dailyDataUsageMB = (m_dailyBytesReceived + m_dailyBytesSent) / (1024 * 1024);
}

void SysInfoMonitor::updateFPS(SysInfo& info)
{
    info.fps = -1.0; // Use a special value to indicate N/A
}

void SysInfoMonitor::updateTemperatures(SysInfo& info)
{
    info.cpuTemp = 0.0;
    info.gpuTemp = 0.0;

    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;

    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres)) return;

    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\WMI"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        return;
    }

    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (SUCCEEDED(hres)) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        double maxCpuTemp = 0.0;
        while (pEnumerator)
        {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn) break;
            VARIANT vtProp;
            hr = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
            double tempKelvin = vtProp.uintVal;
            double tempCelsius = (tempKelvin / 10.0) - 273.15;
            if (tempCelsius > maxCpuTemp) maxCpuTemp = tempCelsius;
            VariantClear(&vtProp);
            pclsObj->Release();
        }
        info.cpuTemp = maxCpuTemp;
        pEnumerator->Release();
    }

    pSvc->Release();
    pLoc->Release();

    // GPU Temp (NVAPI/AMD ADL)
    // This part is complex and requires vendor-specific SDKs.
    // For now, we'll display N/A.
    info.gpuTemp = -1.0; // Use a special value to indicate N/A
}


void SysInfoMonitor::updateSystemInfo(SysInfo& info)
{
    // Get number of active processes
    DWORD processIds[1024];
    DWORD bytesNeeded;
    if (EnumProcesses(processIds, sizeof(processIds), &bytesNeeded)) {
        info.activeProcesses = bytesNeeded / sizeof(DWORD);
    }

    // Get system uptime
    ULONGLONG uptimeMs = GetTickCount64();
    info.systemUptime = uptimeMs / (1000.0 * 60.0 * 60.0); // Convert to hours
}