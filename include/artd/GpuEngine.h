#pragma once

#include "artd/gpu_engine.h"
#include "artd/ObjectBase.h"
#include "artd/vecmath.h"
#include <vector>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class Scene;
class DrawableMesh;

/**
 * A structure that describes one of the data layouts in the vertex buffer
 */
#pragma pack(push,4) // this is often the default
    struct GpuVertexAttributes {
        Vec3f position;
        Vec3f normal;
        glm::vec2 uv;
        
        INL static int floatsPerVertex() {
            return((int)(sizeof(GpuVertexAttributes) / sizeof(float)));
        }
    };
#pragma pack(pop)


class DrawableMeshDescriptor {
public:
    DrawableMeshDescriptor() {
        ::memset(this,0,sizeof(*this));
    }
    const char *cacheName;
    const GpuVertexAttributes *vertices;
    uint32_t vertexCount;
    const uint16_t *indices;  // for triangles ( all we support now ) 3 per triangle
    uint32_t indexCount;  // count of indices in index buffer
    
    inline void setCacheName(const char *name) {
        cacheName = name;
    }
    inline void setVertices(const std::vector<GpuVertexAttributes> &vv) {
        vertices = vv.data();
        vertexCount = (uint32_t)vv.size();
    }
    inline void setIndices(const std::vector<uint16_t> &iv) {
        indices = iv.data();
        indexCount = (uint32_t)iv.size();
    }
};

class ARTD_API_GPU_ENGINE GpuEngine
    : public ObjectBase
{
public:

    GpuEngineImpl &impl();

    GpuEngine();
    ~GpuEngine();

    static ObjectPtr<GpuEngine> createInstance(bool headless, int width, int height);
    void setCurrentScene(ObjectPtr<Scene> scene);
    int run();
    
    ObjectPtr<DrawableMesh> createMesh(const DrawableMeshDescriptor &desc);
};

#undef INL

ARTD_END
