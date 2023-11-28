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

    static GpuEngine &getInstance();
    virtual int init(bool headless, int width, int height) = 0;
    void setCurrentScene(ObjectPtr<Scene> scene);
    int run();

};


ARTD_END