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

	Buffer vertexBuffer_ = nullptr;
    uint32_t vBufferSize_ = 0;

    BufferChunk iChunk_;
    BufferChunk vChunk_;

    DrawableMesh();
    ~DrawableMesh();

    INL const BufferChunk &indices() const {
        return(iChunk_);
    }

    INL const BufferChunk &vertices() const {
        return(iChunk_);
    }

};

#undef INL

ARTD_END
