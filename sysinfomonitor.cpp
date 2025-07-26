#include "sysinfomonitor.h"
#include <comutil.h>
#include <QDebug>
#include <QVector>
#include <QSettings>
#include <iomanip>
#include <iomanip>
#include <QStandardPaths>
#include <QDir>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_cpuQuery = nullptr;
    m_diskQuery = nullptr;
    m_gpuQuery = nullptr;
    m_tempQuery = nullptr;
    m_pLocator = nullptr;
    m_pServices = nullptr;
    m_iAdlAdapterCount = 0;
    m_lpAdlAdapterInfo = nullptr;
    
    nvmlInit_v2();
    if (adl_init() == 0) {
        adl_adapter_numberofadapters_get(&m_iAdlAdapterCount);
        if (m_iAdlAdapterCount > 0) {
            m_lpAdlAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * m_iAdlAdapterCount);
            adl_adapter_adapterinfo_get(m_lpAdlAdapterInfo, sizeof(AdapterInfo) * m_iAdlAdapterCount);
        }
    }

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

    // Initialize WMI
    HRESULT hres;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        qWarning() << "Failed to initialize COM library. Error code = 0x" << hex << hres;
    } else {
        hres = CoInitializeSecurity(
            NULL,
            -1,
            NULL,
            NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE,
            NULL
        );

        if (FAILED(hres)) {
            qWarning() << "Failed to initialize security. Error code = 0x" << hex << hres;
            CoUninitialize();
        } else {
            hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, (LPVOID*)&m_pLocator);

            if (FAILED(hres)) {
                qWarning() << "Failed to create IWbemLocator object. Error code = 0x" << hex << hres;
                CoUninitialize();
            } else {
                hres = m_pLocator->ConnectServer(
                    _bstr_t(L"ROOT\\WMI"),
                    NULL,
                    NULL,
                    0,
                    NULL,
                    0,
                    0,
                    &m_pServices
                );

                if (FAILED(hres)) {
                    qWarning() << "Could not connect to WMI namespace. Error code = 0x" << hex << hres;
                    m_pLocator->Release();
                    CoUninitialize();
                }
            }
        }
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::updateStats);
    m_timer->start(1000);

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
    if (m_tempQuery) PdhCloseQuery(m_tempQuery);

    if (m_pServices) m_pServices->Release();
    if (m_pLocator) m_pLocator->Release();
    CoUninitialize();

    if (m_lpAdlAdapterInfo) {
        free(m_lpAdlAdapterInfo);
    }
    adl_shutdown();
    nvmlShutdown();
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

    ULONG bufferSize = 0;
    GetIfTable2(&bufferSize, nullptr);
    PMIB_IF_TABLE2 ifTable = (PMIB_IF_TABLE2) new BYTE[bufferSize];
    if (GetIfTable2(&bufferSize, ifTable) == NO_ERROR) {
        quint64 totalBytesReceived = 0;
        quint64 totalBytesSent = 0;

        for (ULONG i = 0; i < ifTable->NumEntries; ++i) {
            MIB_IF_ROW2 ifRow = ifTable->Table[i];
            if (ifRow.Type == IF_TYPE_ETHERNET_CSMACD || ifRow.Type == IF_TYPE_IEEE80211) {
                totalBytesReceived += ifRow.InOctets;
                totalBytesSent += ifRow.OutOctets;
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
    delete[] (PBYTE)ifTable;

    info.dailyDataUsageMB = (m_dailyBytesReceived + m_dailyBytesSent) / (1024 * 1024);
}

void SysInfoMonitor::updateFPS(SysInfo& info)
{
    // Simple FPS estimation based on system performance
    // This is a rough estimate - real FPS would require hooking into graphics APIs
    QDateTime now = QDateTime::currentDateTime();
    qint64 timeDiff = m_lastFpsUpdate.msecsTo(now);
    
    if (timeDiff >= 1000) { // Update every second
        // Estimate FPS based on GPU load and system performance
        double estimatedFps = 0.0;
        if (info.gpuLoad > 0) {
            // Very rough estimation: assume higher GPU load means gaming/graphics work
            if (info.gpuLoad > 80) estimatedFps = 30 + (100 - info.gpuLoad) * 3; // High load = lower FPS
            else if (info.gpuLoad > 50) estimatedFps = 60 + (80 - info.gpuLoad);
            else if (info.gpuLoad > 20) estimatedFps = 120 - info.gpuLoad;
            else estimatedFps = 0; // Idle
        }
        
        m_currentFps = estimatedFps;
        m_lastFpsUpdate = now;
    }
    
    info.fps = m_currentFps;
}

void SysInfoMonitor::updateTemperatures(SysInfo& info)
{
    if (m_pServices) {
        IEnumWbemClassObject* pEnumerator = NULL;
        HRESULT hres = m_pServices->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT * FROM MSAcpi_ThermalZoneTemperature"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;
            double maxCpuTemp = 0.0;

            while (pEnumerator) {
                hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn) {
                    break;
                }

                VARIANT vtProp;
                hres = pclsObj->Get(L"CurrentTemperature", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hres)) {
                    double temp = (vtProp.uintVal / 10.0) - 273.15;
                    if (temp > maxCpuTemp) {
                        maxCpuTemp = temp;
                    }
                    VariantClear(&vtProp);
                }
                pclsObj->Release();
            }
            info.cpuTemp = maxCpuTemp;
            pEnumerator->Release();
        }
    } else {
        info.cpuTemp = 0.0;
    }

    unsigned int deviceCount = 0;
    if (nvmlDeviceGetCount_v2(&deviceCount) == NVML_SUCCESS) {
        double maxGpuTemp = 0.0;
        for (unsigned int i = 0; i < deviceCount; i++) {
            nvmlDevice_t device;
            if (nvmlDeviceGetHandleByIndex_v2(i, &device) == NVML_SUCCESS) {
                unsigned int temp;
                if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
                    if (temp > maxGpuTemp) {
                        maxGpuTemp = temp;
                    }
                }
            }
        }
        info.gpuTemp = maxGpuTemp;
    } else {
        info.gpuTemp = 0.0;
    }

    if (m_iAdlAdapterCount > 0) {
        double maxGpuTemp = 0.0;
        for (int i = 0; i < m_iAdlAdapterCount; i++) {
            ADLTemperature adlTemp = { 0 };
            adlTemp.iSize = sizeof(ADLTemperature);
            if (adl_overdrive5_temperature_get(m_lpAdlAdapterInfo[i].iAdapterIndex, 0, &adlTemp) == 0) {
                double temp = adlTemp.iTemperature / 1000.0;
                if (temp > maxGpuTemp) {
                    maxGpuTemp = temp;
                }
            }
        }
        if (maxGpuTemp > info.gpuTemp) {
            info.gpuTemp = maxGpuTemp;
        }
    }
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