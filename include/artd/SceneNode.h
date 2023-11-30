#pragma once

#include "artd/SceneObject.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class TransformNode;
class Scene;
class GpuEngine;

class ARTD_API_GPU_ENGINE SceneNode
    : public SceneObject
{
public:
    friend TransformNode;
    friend Scene;
    
protected:
    TransformNode *parent_ = nullptr;

    bool setParent(TransformNode *parent);

    void onAttach(Scene *s);
    void onDetach(Scene *s);
    void attachAllChildren(Scene *s);
    void detachAllChildren(Scene *s);

public:

    ~SceneNode();

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
