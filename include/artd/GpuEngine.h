#pragma once

#include "artd/gpu_engine.h"
#include "artd/ObjectBase.h"
#include "artd/vecmath.h"

ARTD_BEGIN

class GpuEngineImpl;
class Scene;

class ARTD_API_GPU_ENGINE GpuEngine
    : public ObjectBase
{
protected:
    GpuEngineImpl &impl();
public:
    GpuEngine();
    ~GpuEngine();

    static ObjectPtr<GpuEngine> createInstance(bool headless, int width, int height);
    void setCurrentScene(ObjectPtr<Scene> scene);
    int run();
};


ARTD_END