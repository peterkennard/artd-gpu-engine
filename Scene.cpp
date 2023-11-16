#include "./GpuEngineImpl.h"
#include "artd/Scene.h"
#include "artd/LightNode.h"
#include "./MeshNode.h"

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
    rootNode_ = ObjectBase::make<SceneRoot>(this);
}

Scene::~Scene()
{
    lights_.clear();
}

SceneNode *
Scene::addChild(ObjectPtr<SceneNode> child) {
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

void
Scene::removeActiveLight(LightNode *l) {
    for(auto it = lights_.begin(); it != lights_.end(); ++it) {
        if(*it == l) {
            lights_.erase(it);
            return;
        }
    }
}

void
Scene::addActiveLight(LightNode *l) {

    for(int i = (int)lights_.size(); i > 0;) {
        --i;
        if(lights_[i] == l) {
            return;
        }
    }
    lights_.push_back(l);
}

void
Scene::addDrawable(SceneNode *l) {

    for(int i = (int)drawables_.size(); i > 0;) {
        --i;
        if(((SceneNode*)(drawables_[i])) == l) {
            return;
        }
    }
    drawables_.push_back((MeshNode*)l);
}
void
Scene::removeDrawable(SceneNode *l) {
    for(auto it = drawables_.begin(); it != drawables_.end(); ++it) {
        if(*it == (MeshNode *)l) {
            drawables_.erase(it);
            return;
        }
    }
}

void
Scene::onNodeAttached(SceneNode *n) {
    //AD_LOG(print) << "attached " << (void *)n;
    if(n->isDrawable()) {
        addDrawable(n);
    }
    if(dynamic_cast<LightNode*>(n)) {
        addActiveLight((LightNode *)n);
    }
}

void
Scene::onNodeDetached(SceneNode *n) {
    // AD_LOG(print) << "detached " << (void *)n;
    if(n == nullptr) {
        return;
    }
    if(n->isDrawable()) {
        removeDrawable(n);
    }
    else if(dynamic_cast<LightNode*>(n)) {
        removeActiveLight((LightNode *)n);
    }
}


ARTD_END


