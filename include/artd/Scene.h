#pragma once
#include "artd/TransformNode.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class GpuEngine;

class ARTD_API_GPU_ENGINE Scene
    : protected SceneNode
{
    GpuEngine *owner_;
    friend class GpuEngineImpl;
protected:
    GpuEngineImpl *getOwner();
    typedef SceneNode super;
public:
    Scene(GpuEngine *owner);
    virtual ~Scene();

    SceneNode *addSceneChild(ObjectPtr<SceneNode> child);

    INL GpuEngine *getEngine() {
        return(owner_);
    }

};

INL
Scene *SceneNode::getScene() const {
    if(testFlags(fHasParent))
        return(((SceneNode *)parent_)->getScene());
    return(reinterpret_cast<Scene *>(parent_));
}

#undef INL

ARTD_END
