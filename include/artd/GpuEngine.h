#pragma once

#include "artd/gpu_engine.h"
#include "artd/ObjectBase.h"

ARTD_BEGIN

class ARTD_API_GPU_ENGINE GpuEngine
    : public ObjectBase
{
public:
    GpuEngine();
    ~GpuEngine();
};


ARTD_END