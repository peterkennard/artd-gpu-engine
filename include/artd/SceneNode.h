#pragma once

#include "artd/gpu_engine.h"
#include "artd/Matrix4f.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class TransformNode;
class Scene;
class GpuEngine;

class ARTD_API_GPU_ENGINE SceneNode
{
public:
    static const int32_t fHasChildren = 0x01;
    static const int32_t fHasTransform = 0x02;
    static const int32_t fHasParent    = 0x04;

    friend TransformNode;
protected:
    TransformNode *parent_ = nullptr;
    int32_t flags_ = 0;
    int32_t id_ = -1;

    INL bool setParent(TransformNode *parent) {
        if(parent != parent_) {
            parent_ = parent;
            if(parent) {
                setFlags(fHasParent);
            } else {
                clearFlags(fHasParent);
            }
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
    virtual ~SceneNode()
    {}

    // TODO: have IDs come from a registrar to keep ordered ans searchable
    INL void setId(int32_t id) {
        id_ = id;
    }
    INL int32_t getId() const {
        return(id_);
    }

    INL bool hasChildren() const {
        return(flags_ & fHasChildren);
    }
    INL bool hasParent() const {
        return(flags_ & fHasParent);
    }
    INL bool hasTransform() const {
        return(flags_ & fHasTransform);
    }
    INL TransformNode *getParent() const {
        if(flags_ & fHasParent) {
            return(parent_);
        }
        return(nullptr);
    }
    Scene *getScene() const;
    GpuEngine *getEngine() const;
};

#undef INL

ARTD_END
