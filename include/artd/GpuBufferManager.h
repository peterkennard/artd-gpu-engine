//
// Created by peterk on 3/14/19.
//

#ifndef __artd_GLBufferManager_h
#define __artd_GLBufferManager_h

#include "artd/gpu_engine.h"
//#include "ResourceManager.h"
#include <vector>
#include <map>

ARTD_BEGIN

class BufferHandle;
class GraphicsContext;
class BufferDataImpl;

#define INL ARTD_ALWAYS_INLINE

class ManagedGpuBuffer;

class GpuBufferManager
{
    friend class BufferDataImpl;

    std::vector<ManagedGpuBuffer*> gpuBuffers_;

    static BufferDataImpl *getHandleData(BufferHandle &h);

    ManagedGpuBuffer *createManagedBuffer(int size,int type);

public:
    ~GpuBufferManager();

    void onContextCreated();
    void shutdown();

    BufferHandle createBuffer();
    void allocOrRealloc(BufferHandle &buf, int newSize, int type);
    void loadBuffer(BufferHandle &buf, const void *data, int size, int type);
};

#undef INL

ARTD_END

#endif //__artd_GLBufferManager_h
