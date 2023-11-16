#include "artd/Scene.h"

ARTD_BEGIN

SceneNode::~SceneNode() {
    if(hasParent()) {
        parent_->removeChild(this);
    }
    flags_ = 0;
}


void
SceneNode::onAttach(Scene *s) {
    if(s) {
        s->onNodeAttached(this);
    }
}

void
SceneNode::onDetach(Scene *s) {
    if(s) {
        s->onNodeDetached(this);
    }
}

bool
SceneNode::setParent(TransformNode *parent) {

    // this is set up so if an item is detached it will still have the subtree root
    // attached to the scene and getScene() will work until the object is attached to another
    // parent.  So destructors etc will be able to notify the scene.
    if(parent != parent_) {

        Scene *oldScene = getScene();
        Scene *newScene = nullptr;

        if(parent) {
            setFlags(fHasParent);
            parent_ = parent;
            newScene = parent->getScene();
        } else {
            clearFlags(fHasParent);
            parent_ = reinterpret_cast<TransformNode *>(oldScene);
        }
        if(oldScene != newScene) {
            if(oldScene) {
                
                onDetach(oldScene);
            }
            if(newScene) {
                onAttach(newScene);
            }
        }
        return(true);
    }
    return(false);
}

ARTD_END
