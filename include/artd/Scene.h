#pragma once
#include "artd/TransformNode.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class GpuEngine;
class LightNode;
class Material;
class TimingContext;

class ARTD_API_GPU_ENGINE Scene
{
    GpuEngine *owner_;
    friend class GpuEngineImpl;
    
    ObjectPtr<TransformNode> rootNode_;
    
protected:
    GpuEngineImpl *getOwner();
    typedef SceneNode super;
    
    void tickAnimations(TimingContext &timing);

    
public:

    // scene graph items
    // TODO: active vs: turned off ( visible invisible ) with or without parents ...
    std::vector<LightNode*> lights_;

    Scene(GpuEngine *owner);
    virtual ~Scene();

    // add a child to the root of the scene graph.
    SceneNode *addChild(ObjectPtr<SceneNode> child);
    // add a child to the root of the scene graph.
    void removeChild(SceneNode *child);

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

INL
GpuEngine *SceneNode::getEngine() const {
    // TODO: if too much of a preformance issue ( seriously deep trees ? )
    // maybe needs to be a field.
    return(getScene()->getEngine());
}

#undef INL

ARTD_END
