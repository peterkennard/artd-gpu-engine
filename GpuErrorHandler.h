#pragma once

#include "artd/gpu_engine.h"
#include <webgpu/webgpu.hpp>

ARTD_BEGIN

class GpuErrorHandler {
public:

    static std::unique_ptr<ErrorCallback> initErrorCallback(Device device)  {

        return(device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
            AD_LOG(error) << "Device error: type " << type << "(" << (message ? message : "" ) << ")";
            return;
        }));
    }
};



ARTD_END



