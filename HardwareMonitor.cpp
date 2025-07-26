
#include "HardwareMonitor.h"
#include <msclr/auto_gcroot.h>

// The /FU compiler flag in CMakeLists.txt handles the assembly reference.

using namespace System;
using namespace LibreHardwareMonitor::Hardware;

// Helper function to convert from System::String to std::string
std::string toStdString(System::String^ netString) {
    if (netString == nullptr) return "";
    const char* chars = (const char*)(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(netString)).ToPointer();
    std::string os = chars;
    System::Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr((void*)chars));
    return os;
}

// The actual implementation of the HardwareMonitor, using the PImpl idiom
class HardwareMonitorImpl {
public:
    // Use auto_gcroot to manage the .NET object on the native heap
    msclr::auto_gcroot<Computer^> computer;

    HardwareMonitorImpl() {
        computer = gcnew Computer();
    }
};

// --- Public Interface Implementation ---

HardwareMonitor::HardwareMonitor() {
    pImpl = new HardwareMonitorImpl();
}

HardwareMonitor::~HardwareMonitor() {
    delete pImpl;
}

bool HardwareMonitor::open() {
    try {
        pImpl->computer->IsCpuEnabled = true;
        pImpl->computer->IsGpuEnabled = true;
        pImpl->computer->Open();
        return true;
    }
    catch (Exception^ e) {
        Console::WriteLine("Error opening LHM: " + e->Message);
        return false;
    }
}

void HardwareMonitor::close() {
    pImpl->computer->Close();
}

void HardwareMonitor::update() {
    // To update the sensor values, we need to iterate through the hardware
    // and call the Update() method on each component.
    for each (IHardware^ hardware in pImpl->computer->Hardware) {
        hardware->Update();
        for each (IHardware^ subHardware in hardware->SubHardware) {
            subHardware->Update();
        }
    }
}

std::vector<SensorData> HardwareMonitor::getTemperatureSensors() {
    std::vector<SensorData> sensors;

    for each (IHardware^ hardware in pImpl->computer->Hardware) {
        for each (ISensor^ sensor in hardware->Sensors) {
            if (sensor->SensorType == SensorType::Temperature) {
                SensorData data;
                data.name = toStdString(sensor->Name);
                data.identifier = toStdString(sensor->Identifier->ToString());
                if (sensor->Value.HasValue) {
                    data.value = sensor->Value.Value;
                } else {
                    data.value = -1.0f; // Indicate invalid value
                }
                sensors.push_back(data);
            }
        }
    }
    return sensors;
}
