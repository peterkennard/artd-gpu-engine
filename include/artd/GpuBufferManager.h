#pragma once
//
// Created by peterk on 3/14/19.
//

#include <webgpu/webgpu.hpp>

#include "artd/gpu_engine.h"
#include "artd/ObjectBase.h"

ARTD_BEGIN

using namespace wgpu;

class GpuEngineImpl;
class BufferHandle;
class GraphicsContext;
class BufferDataImpl;

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class ManagedGpuBuffer;

class BufferChunk {
public:
    int64_t start;
    int64_t size;

    INL operator bool() {
        return(size > 0);
    }
    INL bool operator !() {
        return(size <= 0);
    }
};

class ARTD_API_GPU_ENGINE GpuBufferManager
{
    friend class BufferDataImpl;
protected:
    GpuEngineImpl &owner_;
    GpuBufferManager(GpuEngineImpl *owner);

    Buffer storage_ = nullptr;
    Buffer indices_ = nullptr;
    Buffer vertices_ = nullptr;

public:
    static ObjectPtr<GpuBufferManager> create(GpuEngineImpl *owner);

    virtual ~GpuBufferManager();

    virtual BufferChunk allocIndexChunk(int count, const uint16_t *data) = 0;
    virtual BufferChunk allocVertexChunk(int count, const float *data) = 0;
    virtual wgpu::Buffer initStorageBuffer() = 0;

    INL wgpu::Buffer getIndexBuffer() {
        return(indices_);
    }
    
    INL wgpu::Buffer getVertexBuffer() {
        return(vertices_);
    }

    // TODO: for now this is get THE storage buffer
    INL wgpu::Buffer getStorageBuffer() {
        if(!storage_) {
            initStorageBuffer();
        }
        return(storage_);
    }

    virtual void shutdown() = 0;

    void onContextCreated();

    BufferHandle createBuffer();
    void allocOrRealloc(BufferHandle &buf, int newSize, int type);
    void loadBuffer(BufferHandle &buf, const void *data, int size, int type);
};

#undef INL

ARTD_END
