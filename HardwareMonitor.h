
#ifndef HARDWAREMONITOR_H
#define HARDWAREMONITOR_H

#include <string>
#include <vector>

// A simple struct to hold sensor data
struct SensorData {
    std::string name;
    std::string identifier;
    float value;
};

class HardwareMonitorImpl; // Forward declaration for the implementation class

class HardwareMonitor {
public:
    HardwareMonitor();
    ~HardwareMonitor();

    bool open();
    void close();

    // Gets all temperature sensors found
    std::vector<SensorData> getTemperatureSensors();
    
    // Updates the values for all sensors
    void update();

private:
    HardwareMonitorImpl* pImpl; // Pointer to the implementation
};

#endif // HARDWAREMONITOR_H
