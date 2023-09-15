#pragma once

#include "artd/gpu_engine.h"
#include "artd/Matrix4f.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class TransformNode;

class ARTD_API_GPU_ENGINE TransformChild
{
protected:
    TransformNode *parent_ = nullptr;
public:
    INL TransformNode *getParent() { return(parent_); }
};

#undef INL

ARTD_END
