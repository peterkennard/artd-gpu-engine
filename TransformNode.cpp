#include "artd/TransformNode.h"
#include "artd/SceneNode.h"

ARTD_BEGIN

TransformNode::TransformNode() {
    setFlags(fHasTransform);
}

TransformNode::~TransformNode() {
    for(auto i = sceneChildren_.size(); i > 0;) {
        --i;
        clearFlags(fHasChildren);
        sceneChildren_[i]->setParent(nullptr);
    }
    sceneChildren_.clear();
}

bool
TransformNode::removeChild(SceneNode *child) {
    if(child != nullptr) {
        for(auto i = sceneChildren_.size(); i > 0;) {
            --i;
            if(sceneChildren_[i].get() == child) {
                sceneChildren_[i] = nullptr;  // dereference
                sceneChildren_.erase(sceneChildren_.begin() + i);
                child->setParent(nullptr);
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

void
TransformNode::addChild(ObjectPtr<SceneNode> child) {
    if(child) {
        TransformNode *parent = child->getParent();
        if(parent != getParent()) {
            if(parent != nullptr) {
                parent->removeChild(child);
            }
            sceneChildren_.push_back(child);
            child->setParent(this);
            setFlags(fHasChildren);
            setChildrenWorldTransformModified();
        }
    }
}


ARTD_END
