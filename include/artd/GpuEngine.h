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
public:
    /**
     * A structure that describes one of the data layouts in the vertex buffer
     */
    #pragma pack(push,4) // this is often the default
        struct VertexAttributes {
            Vec3f position;
            Vec3f normal;
            glm::vec2 uv;
        };
    #pragma pack(pop)


    GpuEngineImpl &impl();

    GpuEngine();
    ~GpuEngine();

    static ObjectPtr<GpuEngine> createInstance(bool headless, int width, int height);
    void setCurrentScene(ObjectPtr<Scene> scene);
    int run();
};


ARTD_END