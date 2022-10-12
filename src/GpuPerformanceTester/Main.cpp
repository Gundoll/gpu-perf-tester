#include <tchar.h>
#include <windows.h>
#include <nvapi.h>
#include <conio.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <list>

using namespace std;

#if defined(_WIN32)
#define CHRONO_TIME_POINT std::chrono::steady_clock::time_point
#elif defined(__linux__)
#define CHRONO_TIME_POINT std::chrono::system_clock::time_point
#endif

struct GPU_LOG {
	CHRONO_TIME_POINT loggingTime;
	NvS32 temperature;
	struct GPU_USAGE {
		NvS32 gpu;
		NvS32 fb;
		NvS32 vid;
		NvS32 bus;
	} usage;
};

FILE* createLogFile(void);
void saveLogs(FILE* fp, list<GPU_LOG>& logs);
bool isRunning = true;

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

	FILE* fp = createLogFile();
	thread thFinish([&]() {
		_getch();
		isRunning = false;
		});

	list<GPU_LOG> logs;
	while (isRunning) {
		GPU_LOG log;
		log.loggingTime = chrono::high_resolution_clock::now();

		NV_GPU_THERMAL_SETTINGS thermal = {};
		thermal.version = NV_GPU_THERMAL_SETTINGS_VER;
		thermal.sensor[0].controller = NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL;
		thermal.sensor[0].target = NVAPI_THERMAL_TARGET_GPU;
		NvAPI_GPU_GetThermalSettings(gpuHandles[0], 0, &thermal);

		log.temperature = thermal.sensor[0].currentTemp;

		//cout << "CURRENT TEMPERATURE: " << thermal.sensor[0].currentTemp << endl;

		NV_GPU_DYNAMIC_PSTATES_INFO_EX status = {};
		status.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;
		NvAPI_GPU_GetDynamicPstatesInfoEx(gpuHandles[0], &status);

		// NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[0] : GPU (graphic engine)
		// NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[1] : FB (frame buffer)
		// NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[2] : VID (video engine)
		// NV_GPU_DYNAMIC_PSTATES_INFO_EX.utilization[3] : BUS (bus interface)
		log.usage.gpu = status.utilization[0].percentage;
		log.usage.fb = status.utilization[1].percentage;
		log.usage.vid = status.utilization[2].percentage;
		log.usage.bus = status.utilization[3].percentage;

		logs.push_back(log);

		this_thread::sleep_for(1s);
	}

	saveLogs(fp, logs);

	fclose(fp);

	return 0;
}

FILE* createLogFile(void) {
	FILE* fp = nullptr;
	time_t currentTime = time(0);
	struct tm* structTime = localtime(&currentTime);

	char logFileName[256] = {};
	sprintf(logFileName, "%04d%02d%02d.%02d%02d.gpu.perf.csv", structTime->tm_year + 1900, structTime->tm_mon + 1, structTime->tm_mday, structTime->tm_hour, structTime->tm_min);
	fopen_s(&fp, logFileName, "w+");

	return fp;
}

void saveLogs(FILE* fp, list<GPU_LOG>& logs) {
	if (fp == nullptr) {
		return;
	}

	fprintf(fp, "time,temperature,GPU(graphic engine),FB(frame buffer),VID(video engine),BUS(bus interface)\n");
	for (GPU_LOG& log : logs) {
		fprintf(fp, "%lld,%d,%d,%d,%d,%d\n", log.loggingTime, log.temperature, log.usage.gpu, log.usage.fb, log.usage.vid, log.usage.bus);
	}
}