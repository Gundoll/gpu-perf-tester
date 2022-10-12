#include <tchar.h>
#include <windows.h>
#include <nvapi.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

int main(int argc, _TCHAR* argv[]) {
    NvAPI_Status ret = NVAPI_OK;
    ret = NvAPI_Initialize();
    if (ret != NVAPI_OK) {
        printf("NvAPI_Initialize() failed = 0x%x", ret);
        return 1; // Initialization failed
    }

    NvU32 gpuCount = 0;
    NvPhysicalGpuHandle gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
    ret = NvAPI_EnumPhysicalGPUs(gpuHandles, &gpuCount);
    if (ret != NVAPI_OK) {
        printf("NvAPI_EnumPhysicalGPUs() failed = 0x%x", ret);
        return 1; // Initialization failed
    }

    //cout << "GPU COUNT: " << gpuCount << endl;

    while (true) {
        NV_GPU_THERMAL_SETTINGS thermal = {};
        thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
        thermal.sensor[0].controller = NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL;
        thermal.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;
        NvAPI_GPU_GetThermalSettings(gpuHandles[0], 0, &thermal);

        //cout << "CURRENT TEMPERATURE: " << thermal.sensor[0].currentTemp << endl;

        NV_GPU_DYNAMIC_PSTATES_INFO_EX status = {};
        status.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
        NvAPI_GPU_GetDynamicPstatesInfoEx(gpuHandles[0], &status);

        size_t totalUsage = 0;
        // NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[0] : GPU (graphic engine)
        // NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[1] : FB (frame buffer)
        // NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[2] : VID (video engine)
        // NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[3] : BUS (bus interface)
        for (size_t idx = 0; idx < NVAPI_MAX_GPU_UTILIZATIONS; idx++) {
            totalUsage += status.utilization[idx].percentage;
            //printf("  [%02d] %d, %d\n", idx, status.utilization[idx].bIsPresent, status.utilization[idx].percentage);
        }
        //cout << "CURRENT USAGE (TOTAL): " << totalUsage << endl;

        this_thread::sleep_for(1s);
    }

    return 0;
}
