#include "./GpuEngineImpl.h"
#include "artd/Scene.h"
#include "artd/LightNode.h"

ARTD_BEGIN

class SceneRoot
    : public TransformNode
{
public:
    SceneRoot(Scene *owner) {
        parent_ = reinterpret_cast<TransformNode *>(owner);
        clearFlags(fHasTransform | fHasParent);
        setId(-1);
    }
};

Scene::Scene(GpuEngine *e)
    : owner_(e)
{
    rootNode_ = ObjectBase::make<TransformNode>();
}

Scene::~Scene()
{
}

SceneNode *
Scene::addChild(ObjectPtr<SceneNode> child) {
    if(dynamic_cast<LightNode*>(child.get())) {
        lights_.push_back(static_cast<LightNode*>(child.get()));
    }
    return(rootNode_->addChild(child));
}

void
Scene::removeChild(SceneNode *child) {
    
    // TODO: we need to update the light list
    // when a light is removed - even if attached to a
    // lower down the tree node.
    
    rootNode_->removeChild(child);
}

void
Scene::tickAnimations(TimingContext &timing) {
    timing.isDebugFrame();
}


ARTD_END


