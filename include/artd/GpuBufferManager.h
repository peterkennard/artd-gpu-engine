#pragma once
//
// Created by peterk on 3/14/19.
//

#include <webgpu/webgpu.hpp>

#include "artd/gpu_engine.h"
#include "artd/ObjectBase.h"
#include "artd/pointer_math.h"

ARTD_BEGIN

using namespace wgpu;

class GpuEngineImpl;
class GraphicsContext;
class BufferDataImpl;

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class ManagedGpuBuffer;

class BufferChunk {
    uint64_t start_;
    uint32_t size_;
public:
    
    INL BufferChunk()
        : start_(0)
        , size_(0)
    {}
    
    INL BufferChunk(uint64_t start, uint32_t size)
        : start_(start)
        , size_(size)
    {}
    INL operator bool() const {
        return(size_ != 0);
    }
    INL bool operator !() const {
        return(size_ == 0);
    }
    INL uint64_t start() const {
        return(start_);
    }
    INL uint32_t size() const {
        return((uint32_t)size_);
    }
    INL uint32_t alignedSize() const {
        return(ARTD_ALIGN_UP(size_,4));
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

//    BufferHandle createBuffer();
//    void allocOrRealloc(BufferHandle &buf, int newSize, int type);
//    void loadBuffer(BufferHandle &buf, const void *data, int size, int type);
};

#undef INL

ARTD_END
