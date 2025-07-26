#include "sysinfomonitor.h"
#include <QDebug>
#include <QVector>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
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
    
    // Initialize FPS tracking
    m_lastFpsUpdate = QDateTime::currentDateTime();
    m_frameCount = 0;
    m_currentFps = 0.0;

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


    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::updateStats);
    m_timer->start(1000);

    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(
        nullptr, -1, nullptr, nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr, EOAC_NONE, nullptr
    );

    HRESULT hres = CoCreateInstance(
        CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&m_pLocator
    );

    if (SUCCEEDED(hres)) {
        hres = m_pLocator->ConnectServer(
            _bstr_t(L"ROOT\\WMI"),
            nullptr, nullptr, 0, nullptr, 0, 0, &m_pServices
        );
    }

    updateStats();
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

    if (m_pServices) m_pServices->Release();
    if (m_pLocator) m_pLocator->Release();
    CoUninitialize();
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
    QDateTime now = QDateTime::currentDateTime();
    qint64 currentDay = getCurrentDayKey();

    if (currentDay != m_currentDayKey) {
        saveDailyUsage();
        m_currentDayKey = currentDay;
        m_dailyBytesReceived = 0;
        m_dailyBytesSent = 0;
    }

    MIB_IF_TABLE* ifTable = nullptr;
    DWORD bufLen = 0;
    if (GetIfTable(ifTable, &bufLen, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        ifTable = (MIB_IF_TABLE*)new char[bufLen];
    }

    if (GetIfTable(ifTable, &bufLen, FALSE) == NO_ERROR) {
        qint64 totalBytesReceived = 0;
        qint64 totalBytesSent = 0;
        for (DWORD i = 0; i < ifTable->dwNumEntries; ++i) {
            MIB_IF_ROW* row = &ifTable->table[i];
            //- row->dwType != MIB_IF_TYPE_LOOPBACK && row->dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED
            if (row->dwType == MIB_IF_TYPE_ETHERNET_CSMACD || row->dwType == IF_TYPE_IEEE80211) {
                totalBytesReceived += row->dwInOctets;
                totalBytesSent += row->dwOutOctets;
            }
        }

        qint64 timeDiffMs = m_lastNetworkUpdate.msecsTo(now);
        if (timeDiffMs > 0 && m_lastBytesReceived > 0) {
            double timeDiffSec = timeDiffMs / 1000.0;
            info.networkDownloadSpeed = (totalBytesReceived - m_lastBytesReceived) / (1024.0 * 1024.0 * timeDiffSec);
            info.networkUploadSpeed = (totalBytesSent - m_lastBytesSent) / (1024.0 * 1024.0 * timeDiffSec);

            m_dailyBytesReceived += (totalBytesReceived - m_lastBytesReceived);
            m_dailyBytesSent += (totalBytesSent - m_lastBytesSent);
        }

        m_lastBytesReceived = totalBytesReceived;
        m_lastBytesSent = totalBytesSent;
        m_lastNetworkUpdate = now;
    }

    if (ifTable) {
        delete[] ifTable;
    }

    info.dailyDataUsageMB = (m_dailyBytesReceived + m_dailyBytesSent) / (1024 * 1024);
}

void SysInfoMonitor::updateFPS(SysInfo& info)
{
    // FPS monitoring is complex and requires hooking into graphics APIs (e.g., DirectX, OpenGL).
    // This is a placeholder for where such an implementation would go.
    info.fps = 0.0;
}

void SysInfoMonitor::updateTemperatures(SysInfo& info)
{
    // CPU Temperature (WMI)
    IEnumWbemClassObject* pEnumerator = nullptr;
    HRESULT hres = m_pServices->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (SUCCEEDED(hres)) {
        IWbemClassObject* pclsObj = nullptr;
        ULONG uReturn = 0;
        double maxCpuTemp = 0.0;

        while (pEnumerator) {
            hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (uReturn == 0) {
                break;
            }

            VARIANT vtProp;
            hres = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hres)) {
                double tempKelvin = vtProp.uintVal;
                double tempCelsius = (tempKelvin / 10.0) - 273.15;
                if (tempCelsius > maxCpuTemp && tempCelsius < 150) {
                    maxCpuTemp = tempCelsius;
                }
            }
            pclsObj->Release();
        }
        info.cpuTemp = maxCpuTemp;
        pEnumerator->Release();
    }

    // GPU Temperature (requires vendor-specific APIs like NVAPI or AGS)
    // Placeholder for GPU temperature
    info.gpuTemp = 0.0;
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