#pragma once
#include "artd/TransformNode.h"
#include <webgpu/webgpu.hpp>

ARTD_BEGIN

using namespace wgpu;

#define INL ARTD_ALWAYS_INLINE

class DrawableMesh
{
public:

	Buffer vertexBuffer_ = nullptr;
    uint32_t vBufferSize_ = 0;

	Buffer indexBuffer_ = nullptr;
    uint32_t iBufferSize_ = 0;

    DrawableMesh();
    ~DrawableMesh();

    INL void setVertices(Buffer vb) {
        vertexBuffer_ = vb;
    }

    INL uint32_t verticesSize() {
        return(vBufferSize_);
    }
    
    INL Buffer getVertices() {
        return(vertexBuffer_);
    }

    INL uint32_t indicesSize() {
        return(iBufferSize_);
    }

    INL void setIndices(Buffer ib) {
        indexBuffer_ = ib;
    }

    INL Buffer getIndices() {
        return(indexBuffer_);
    }
};

#undef INL

ARTD_END
