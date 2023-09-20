#pragma once
//
// Created by peterk on 3/14/19.
//

#include "artd/gpu_engine.h"
//#include "ResourceManager.h"
#include "artd/ObjectBase.h"

ARTD_BEGIN

class GpuEngineImpl;
class BufferHandle;
class GraphicsContext;
class BufferDataImpl;

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class ManagedGpuBuffer;

class ARTD_API_GPU_ENGINE GpuBufferManager
{
    friend class BufferDataImpl;
protected:
    GpuEngineImpl &owner_;
    GpuBufferManager(GpuEngineImpl *owner);
public:
    static ObjectPtr<GpuBufferManager> create(GpuEngineImpl *owner);

    virtual ~GpuBufferManager();

    void onContextCreated();
    virtual void shutdown() = 0;

    BufferHandle createBuffer();
    void allocOrRealloc(BufferHandle &buf, int newSize, int type);
    void loadBuffer(BufferHandle &buf, const void *data, int size, int type);
};

#undef INL

ARTD_END
