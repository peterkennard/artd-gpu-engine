#pragma once

#include "artd/gpu_engine.h"
#include "artd/Matrix4f.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class TransformNode;

class ARTD_API_GPU_ENGINE SceneNode
{
public:
    static const int32_t fHasChildren = 0x01;
    static const int32_t fHasTransform = 0x02;
    friend TransformNode;
protected:
    TransformNode *parent_ = nullptr;
    int32_t flags_ = 0;

    INL bool setParent(TransformNode *parent) {
        if(parent != parent_) {
            parent_ = parent;
            return(true);
        }
        return(false);
    }
    INL void setFlags(uint32_t flags) {
        flags_ |= flags;
    }
    INL void clearFlags(uint32_t flags) {
        flags_ &= ~flags;
    }
    INL int32_t testFlags(int flags) const {
        return(flags_ & flags);
    }
public:

    INL bool hasChildren() const {
        return(flags_ & fHasChildren);
    }
    INL bool hasTransform() const {
        return(flags_ & fHasTransform);
    }
    INL TransformNode *getParent() const {
        return(parent_);
    }
};

#undef INL

ARTD_END
