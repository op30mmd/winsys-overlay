#include "nvml.h"
#include <windows.h>

typedef nvmlReturn_t (*nvmlInit_t)(void);
typedef nvmlReturn_t (*nvmlShutdown_t)(void);
typedef nvmlReturn_t (*nvmlDeviceGetCount_t)(unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature_t)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int*);

static HMODULE nvml_dll;
static nvmlInit_t nvmlInit_ptr;
static nvmlShutdown_t nvmlShutdown_ptr;
static nvmlDeviceGetCount_t nvmlDeviceGetCount_ptr;
static nvmlDeviceGetHandleByIndex_t nvmlDeviceGetHandleByIndex_ptr;
static nvmlDeviceGetTemperature_t nvmlDeviceGetTemperature_ptr;

nvmlReturn_t nvmlInit_v2(void) {
    nvml_dll = LoadLibraryA("nvml.dll");
    if (nvml_dll == NULL) {
        return NVML_ERROR_LIBRARY_NOT_FOUND;
    }

    nvmlInit_ptr = (nvmlInit_t)GetProcAddress(nvml_dll, "nvmlInit_v2");
    nvmlShutdown_ptr = (nvmlShutdown_t)GetProcAddress(nvml_dll, "nvmlShutdown");
    nvmlDeviceGetCount_ptr = (nvmlDeviceGetCount_t)GetProcAddress(nvml_dll, "nvmlDeviceGetCount_v2");
    nvmlDeviceGetHandleByIndex_ptr = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(nvml_dll, "nvmlDeviceGetHandleByIndex_v2");
    nvmlDeviceGetTemperature_ptr = (nvmlDeviceGetTemperature_t)GetProcAddress(nvml_dll, "nvmlDeviceGetTemperature");

    if (nvmlInit_ptr == NULL || nvmlShutdown_ptr == NULL || nvmlDeviceGetCount_ptr == NULL ||
        nvmlDeviceGetHandleByIndex_ptr == NULL || nvmlDeviceGetTemperature_ptr == NULL) {
        FreeLibrary(nvml_dll);
        return NVML_ERROR_FUNCTION_NOT_FOUND;
    }

    return nvmlInit_ptr();
}

nvmlReturn_t nvmlShutdown(void) {
    if (nvml_dll) {
        nvmlReturn_t result = nvmlShutdown_ptr();
        FreeLibrary(nvml_dll);
        return result;
    }
    return NVML_SUCCESS;
}

nvmlReturn_t nvmlDeviceGetCount_v2(unsigned int* deviceCount) {
    if (nvmlDeviceGetCount_ptr) {
        return nvmlDeviceGetCount_ptr(deviceCount);
    }
    return NVML_ERROR_UNINITIALIZED;
}

nvmlReturn_t nvmlDeviceGetHandleByIndex_v2(unsigned int index, nvmlDevice_t* device) {
    if (nvmlDeviceGetHandleByIndex_ptr) {
        return nvmlDeviceGetHandleByIndex_ptr(index, device);
    }
    return NVML_ERROR_UNINITIALIZED;
}

nvmlReturn_t nvmlDeviceGetTemperature(nvmlDevice_t device, nvmlTemperatureSensors_t sensorType, unsigned int* temp) {
    if (nvmlDeviceGetTemperature_ptr) {
        return nvmlDeviceGetTemperature_ptr(device, sensorType, temp);
    }
    return NVML_ERROR_UNINITIALIZED;
}
