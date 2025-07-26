
#include "HardwareMonitor.h"
#include <msclr/auto_gcroot.h>

// Reference the LHM .NET assembly
#using <C:\Users\mmd\Documents\winsys-overlay\libs\lhm\LibreHardwareMonitorLib.dll>

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
    // This is a visitor that will traverse all hardware and sensors
    // In LHM, you need to call Accept on the computer object to update the sensor values.
    // We create a dummy visitor because we don't need to do anything special during the update.
    Visitor^ visitor = gcnew Visitor();
    pImpl->computer->Accept(visitor);
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
