
#include "sysinfomonitor.h"
#include <QDebug>

SysInfoMonitor::SysInfoMonitor(QObject *parent) : QObject(parent)
{
    m_hardwareMonitor = new HardwareMonitor();
    m_thread = new QThread(this);
    m_timer = new QTimer(0); // Create a timer that will live in the new thread

    // When the thread starts, it will call our initialize method
    connect(m_thread, &QThread::started, this, &SysInfoMonitor::initialize);
    
    // Move this object to the new thread
    moveToThread(m_thread);
}

SysInfoMonitor::~SysInfoMonitor()
{
    stop();
    delete m_hardwareMonitor;
}

void SysInfoMonitor::start() {
    m_thread->start();
}

void SysInfoMonitor::stop() {
    if (m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void SysInfoMonitor::initialize() {
    // This method is now running on the background thread
    if (!m_hardwareMonitor->open()) {
        qWarning() << "Failed to open Libre Hardware Monitor.";
        return;
    }

    // Configure the timer to fire every second for polling
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &SysInfoMonitor::poll);
    m_timer->start();
}

void SysInfoMonitor::poll() {
    // This is the main polling loop on the background thread
    m_hardwareMonitor->update();

    // Reset the struct to avoid stale data
    m_sysInfo = {0};

    auto sensors = m_hardwareMonitor->getTemperatureSensors();
    for (const auto& sensor : sensors) {
        // Find the CPU and GPU temperatures from the sensor list
        // This is a simplified example; a real implementation would be more robust
        if (sensor.identifier.find("/intelcpu/0/temperature") != std::string::npos) {
            if (sensor.name.find("Core") != std::string::npos && sensor.name.find("Max") != std::string::npos) {
                 m_sysInfo.cpuTemp = sensor.value;
            }
        } else if (sensor.identifier.find("/nvidiagpu/0/temperature") != std::string::npos) {
            if (sensor.name == "GPU Core") {
                m_sysInfo.gpuTemp = sensor.value;
            }
        } else if (sensor.identifier.find("/amdgpu/0/temperature") != std::string::npos) {
            if (sensor.name == "GPU Core") {
                m_sysInfo.gpuTemp = sensor.value;
            }
        }
    }

    // Emit the updated stats to the main GUI thread
    emit statsUpdated(m_sysInfo);
}
