#pragma once

#include "artd/gpu_engine.h"
#include "artd/TimingContext.h"
#include "artd/Logger.h"
#include <algorithm>
#include <iostream>

ARTD_BEGIN

class FpsMonitor {

    double  updateInterval;
    double  fpsStartTime;
    double  fpsLastUpdateTime, fpsLastPeriod, fpsTotalDuration;
    int     fpsTotalFrames;
    int     fpsIntervalFrames;
    float   fpsLast,  fpsTotal;

public:
    FpsMonitor() {
        fpsStartTime = -1.0;
        updateInterval = 3.5;
    }

    void tickFrame(const TimingContext &tc) {

        double now = tc.frameTime();

        if(fpsStartTime == -1.0) {
            fpsStartTime = now - .0000001;
            fpsTotalDuration = 0;
            fpsTotalFrames = 0;
            fpsIntervalFrames = 0;
            fpsTotal = 0;
            fpsLastUpdateTime = fpsStartTime;
            fpsLastPeriod = 0;
            fpsLast = 0;
            fpsTotal = 0;
            return;
        }

        ++fpsIntervalFrames;
        fpsLastPeriod = now - fpsLastUpdateTime;

        if(fpsLastPeriod > updateInterval) {
            fpsLast = ((double)fpsIntervalFrames) / fpsLastPeriod;
            fpsLastPeriod = 0;
            fpsTotalFrames += fpsIntervalFrames;
            fpsIntervalFrames = 0;
            fpsTotalDuration = now - fpsStartTime;
            fpsTotal = ((double)fpsTotalFrames) / fpsTotalDuration;
            fpsLastUpdateTime = now - .0000001;
            AD_LOG(print) << "FPS: " << std::fixed << std::setprecision(2) << fpsLast;
        }
    }
};


ARTD_END
