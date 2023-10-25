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

class BufferChunk {
protected:

    Buffer *parent_;  // note this doubles as pointer to parent "Managed Buffer"
    uint32_t size_;
    uint32_t start_;  // 1/4 start offset

    INL BufferChunk()
        : parent_(nullptr)
        , size_(0)
        , start_(0)
    {}
    
    INL BufferChunk(Buffer *buf, uint64_t start, uint32_t size)
        : parent_(buf)
        , size_(size)
        , start_((uint32_t)(start >> 2))
    {}
public:
    
    ~BufferChunk();
    
    INL operator bool() const {
        return(size_ != 0);
    }
    INL bool operator !() const {
        return(size_ == 0);
    }
    INL uint64_t getStartOffset() const {
        return(start_ << 2);
    }
    INL uint32_t getSize() const {
        return(size_);
    }
//    INL uint32_t alignedSize() const {
//        return(ARTD_ALIGN_UP(size_,4));
//    }
    INL Buffer getBuffer() const {
        if(parent_)
            return(*parent_);
        return(nullptr);
    }
};


class ARTD_API_GPU_ENGINE GpuBufferManager
{
    friend class BufferDataImpl;
protected:
    GpuEngineImpl &owner_;
    GpuBufferManager(GpuEngineImpl *owner);
public:

    static ObjectPtr<GpuBufferManager> create(GpuEngineImpl *owner);

    virtual ~GpuBufferManager();

    virtual ObjectPtr<BufferChunk> allocUniformChunk(uint32_t size) = 0;
    virtual ObjectPtr<BufferChunk> allocStorageChunk(uint32_t size) = 0;
    virtual ObjectPtr<BufferChunk> allocIndexChunk(int count, const uint16_t *data) = 0;
    virtual ObjectPtr<BufferChunk> allocVertexChunk(int count, const float *data) = 0;

    virtual void shutdown() = 0;

    void onContextCreated();

//    BufferHandle createBuffer();
//    void allocOrRealloc(BufferHandle &buf, int newSize, int type);
//    void loadBuffer(BufferHandle &buf, const void *data, int size, int type);
};


#undef INL

ARTD_END
