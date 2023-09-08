#pragma once

#ifndef __JEXTRACTING__
    #include "gpu_engine.h"
#else
	#define ARTD_API_GPU_ENGINE
#endif

#ifdef __cplusplus
extern "C" {
#endif

    ARTD_API_GPU_ENGINE int initGPUTest(int width, int height);
    ARTD_API_GPU_ENGINE int renderGPUTestFrame();
    ARTD_API_GPU_ENGINE void shutdownGPUTest();
    ARTD_API_GPU_ENGINE int runGpuTest(int argc, char**argv);
    ARTD_API_GPU_ENGINE int getPixels(int *pBuf);
    ARTD_API_GPU_ENGINE const int *lockPixels(int timeoutMillis);
    ARTD_API_GPU_ENGINE void unlockPixels();

#ifdef __cplusplus
}
#endif
