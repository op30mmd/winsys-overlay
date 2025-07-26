#include "adl.h"
#include <windows.h>

static HMODULE adl_dll;
static ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;
static ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy;
static ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get;
static ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get;
static ADL_OVERDRIVE5_TEMPERATURE_GET ADL_Overdrive5_Temperature_Get;

static void* ADL_API adl_malloc(int size) {
    return malloc(size);
}

int adl_init(void) {
    adl_dll = LoadLibraryA("atiadlxx.dll");
    if (adl_dll == NULL) {
        adl_dll = LoadLibraryA("atiadlxy.dll");
    }
    if (adl_dll == NULL) {
        return -1;
    }

    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(adl_dll, "ADL_Main_Control_Create");
    ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(adl_dll, "ADL_Main_Control_Destroy");
    ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(adl_dll, "ADL_Adapter_NumberOfAdapters_Get");
    ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(adl_dll, "ADL_Adapter_AdapterInfo_Get");
    ADL_Overdrive5_Temperature_Get = (ADL_OVERDRIVE5_TEMPERATURE_GET)GetProcAddress(adl_dll, "ADL_Overdrive5_Temperature_Get");

    if (ADL_Main_Control_Create == NULL || ADL_Main_Control_Destroy == NULL || ADL_Adapter_NumberOfAdapters_Get == NULL ||
        ADL_Adapter_AdapterInfo_Get == NULL || ADL_Overdrive5_Temperature_Get == NULL) {
        FreeLibrary(adl_dll);
        return -1;
    }

    return ADL_Main_Control_Create(adl_malloc, 1);
}

void adl_shutdown(void) {
    if (adl_dll) {
        ADL_Main_Control_Destroy();
        FreeLibrary(adl_dll);
    }
}

int adl_adapter_numberofadapters_get(int* numAdapters) {
    if (ADL_Adapter_NumberOfAdapters_Get) {
        return ADL_Adapter_NumberOfAdapters_Get(numAdapters);
    }
    return -1;
}

int adl_adapter_adapterinfo_get(LPAdapterInfo info, int size) {
    if (ADL_Adapter_AdapterInfo_Get) {
        return ADL_Adapter_AdapterInfo_Get(info, size);
    }
    return -1;
}

int adl_overdrive5_temperature_get(int adapterIndex, int thermalControllerIndex, LPADLTemperature temperature) {
    if (ADL_Overdrive5_Temperature_Get) {
        return ADL_Overdrive5_Temperature_Get(adapterIndex, thermalControllerIndex, temperature);
    }
    return -1;
}
