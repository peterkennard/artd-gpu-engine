#pragma once
#include "artd/TransformNode.h"
#include <webgpu/webgpu.hpp>
#include "artd/GpuBufferManager.h"

ARTD_BEGIN

using namespace wgpu;

#define INL ARTD_ALWAYS_INLINE

class DrawableMesh
{
public:

    ObjectPtr<BufferChunk> iChunk_;
    ObjectPtr<BufferChunk> vChunk_;
    
    int indexCount_ = 0;

    DrawableMesh();
    virtual ~DrawableMesh();
    virtual const char *getName() const = 0;

    INL const BufferChunk &indices() const {
        return(*iChunk_);
    }

    INL const BufferChunk &vertices() const {
        return(*iChunk_);
    }
    
    INL int indexCount() const {
        return(indexCount_);
    }
    

};

#undef INL

ARTD_END
