#include "artd/TransformNode.h"
#include "artd/SceneNode.h"

ARTD_BEGIN

TransformNode::TransformNode() {
    setFlags(fHasTransform);
}

TransformNode::~TransformNode() {
    setParent(nullptr); // will recursively detach all children from scene if attached
    for(auto i = sceneChildren_.size(); i > 0;) {
        --i;
        sceneChildren_.erase(sceneChildren_.begin() + i);
    }
}

bool
TransformNode::removeChild(SceneNode *child) {
    if(child != nullptr) {
        for(auto i = sceneChildren_.size(); i > 0;) {
            --i;
            if(sceneChildren_[i].get() == child) {
                child->setParent(nullptr);    // will detach from scene recursively if attached
                sceneChildren_.erase(sceneChildren_.begin() + i);
                if(sceneChildren_.size() < 1) {
                    clearFlags(fHasChildren);
                }
                return(true);
            }
        }
    }
    return(false);
}

void
TransformNode::setChildrenWorldTransformModified() {
    for (ObjectPtr<SceneNode> &child : sceneChildren_) {
        if(child->hasTransform()) {
            auto tChild = static_cast<TransformNode*>(child.get());
            tChild->setWorldTransformModified();
        }
    }
}

SceneNode *
TransformNode::addChild(ObjectPtr<SceneNode> child) {
    if(child) {
        TransformNode *parent = child->getParent();
        if(parent != this) {
            if(parent != nullptr) {
                parent->removeChild(child);
            }
            sceneChildren_.push_back(child);
            child->setParent(this);
            setFlags(fHasChildren);
            setChildrenWorldTransformModified();
        }
    }
    return(child.get());
}

void
SceneNode::forAllChildrenInTree(const std::function<bool(SceneNode *child)> &doChild, int flags) {
    if((flags & (DepthFirst | Inclusive)) == (Inclusive)) {
        doChild(this);
    }
    if(hasChildren()) {
        TransformNode *tNode ((TransformNode *)this);
        for(auto i = tNode->sceneChildren_.size(); i > 0;) {
            --i;
            SceneNode &child = *(tNode->sceneChildren_[i]);
            child.forAllChildrenInTree(doChild, flags | Inclusive);
        }
    }
    if((flags & (DepthFirst | Inclusive)) == (DepthFirst | Inclusive)) {
        doChild(this);
    }
}


ARTD_END
