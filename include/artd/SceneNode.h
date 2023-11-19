#pragma once

#include "artd/gpu_engine.h"
#include "artd/Matrix4f.h"
#include "artd/TypedPropertyMap.h"
#include <functional>

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class TransformNode;
class Scene;
class GpuEngine;

class ARTD_API_GPU_ENGINE SceneNode
{
public:
    static const int32_t fHasChildren  = 0x01;
    static const int32_t fHasTransform = 0x02;
    static const int32_t fHasParent    = 0x04;
    static const int32_t fIsDrawable   = 0x08;

    friend TransformNode;
    friend Scene;
    
protected:
    TransformNode *parent_ = nullptr;
    int32_t flags_ = 0;
    int32_t id_ = -1;
    TypedPropertyMap properties_;


    bool setParent(TransformNode *parent);

    INL void setFlags(uint32_t flags) {
        flags_ |= flags;
    }
    INL void clearFlags(uint32_t flags) {
        flags_ &= ~flags;
    }
    INL int32_t testFlags(int flags) const {
        return(flags_ & flags);
    }

    void onAttach(Scene *s);
    void onDetach(Scene *s);
    void attachAllChildren(Scene *s);
    void detachAllChildren(Scene *s);

public:

    TypedPropertyMap &properties() {
        return(properties_);
    }

    template<class T>
    INL TypedPropertyKey<T> registerPropertyKey(StringArg name) {
        return(TypedPropertyKey<T>(name));
    }

    template<class T>
    INL T *getProperty(TypedPropertyKey<T> key) {
        return(properties_.getProperty(key));
    }
    
    virtual ~SceneNode();

    // TODO: have IDs come from a registrar to keep ordered and searchable
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
    INL bool isDrawable() const {
        return(flags_ & fIsDrawable);
    }
    INL TransformNode *getParent() const {
        if(flags_ & fHasParent) {
            return(parent_);
        }
        return(nullptr);
    }
    Scene *getScene() const;
    GpuEngine *getEngine() const;
    
    static const int Inclusive = 0x01;
    static const int DepthFirst = 0x02;
    void forAllChildrenInTree(const std::function<bool(SceneNode *)> &doChild, int flags = (DepthFirst | Inclusive));
};

#undef INL

ARTD_END
