#ifndef ADL_H
#define ADL_H

#ifdef __cplusplus
extern "C" {
#endif

#define ADL_API __stdcall
#define ADL_MAX_PATH 256

typedef void* (ADL_API *ADL_MALLOC_CALLBACK)(int);

typedef struct AdapterInfo
{
    int iSize;
    int iAdapterIndex;
    char strUDID[ADL_MAX_PATH];
    int iBusNumber;
    int iDeviceNumber;
    int iFunctionNumber;
    int iVendorID;
    char strAdapterName[ADL_MAX_PATH];
    char strDisplayName[ADL_MAX_PATH];
    int iPresent;
    int iExist;
    char strDriverPath[ADL_MAX_PATH];
    char strDriverPathExt[ADL_MAX_PATH];
    char strPNPString[ADL_MAX_PATH];
    int iOSDisplayIndex;
} AdapterInfo, *LPAdapterInfo;

typedef struct ADLTemperature
{
    int iSize;
    int iTemperature;
} ADLTemperature, *LPADLTemperature;

typedef int (ADL_API *ADL_MAIN_CONTROL_CREATE)(ADL_MALLOC_CALLBACK, int);
typedef int (ADL_API *ADL_MAIN_CONTROL_DESTROY)();
typedef int (ADL_API *ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
typedef int (ADL_API *ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);
typedef int (ADL_API *ADL_OVERDRIVE5_TEMPERATURE_GET)(int, int, LPADLTemperature);

int adl_init(void);
void adl_shutdown(void);
int adl_adapter_numberofadapters_get(int* numAdapters);
int adl_adapter_adapterinfo_get(LPAdapterInfo info, int size);
int adl_overdrive5_temperature_get(int adapterIndex, int thermalControllerIndex, LPADLTemperature temperature);

#ifdef __cplusplus
}
#endif

#endif // ADL_H
