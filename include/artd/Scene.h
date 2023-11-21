#pragma once
#include "artd/TransformNode.h"
#include "artd/TimingContext.h"
#include <functional>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;
class GpuEngine;
class LightNode;
class Material;
class TimingContext;
class MeshNode;
class AnimationTaskList;

class AnimationTaskContext
{
    SceneNode *owner_;
    friend AnimationTaskList;
protected:
    TimingContext timing_;
    INL void setTiming(TimingContext &t) {
        timing_ = t;
    }
public:
    INL const TimingContext &timing() const {
        return(timing_);
    }
    INL SceneNode *owner() {
        return(owner_);
    }
};



class ARTD_API_GPU_ENGINE AnimationTask
{
public:
//    enum When {
//        /// tick task before input events dispatched. ( post render )
//        TICK_PRE_DISPATCH = 0,
//        /// tick task before after input events dispatched. and before scene update.
//        TICK_POST_DISPATCH = 1,
//        TICK_PRE_UPDATE = TICK_POST_DISPATCH,
//        /// tick task after after scene update and before render.
//        TICK_POST_UPDATE = 2,
//        TICK_PRE_RENDER = TICK_POST_UPDATE,
//        /// convenience name - this is a LOOP !!!
//        TICK_POST_RENDER = TICK_PRE_DISPATCH
//    };

    virtual ~AnimationTask() {}
    virtual bool onAnimationTick(AnimationTaskContext &c) = 0;
};

class AnimationTaskList;

class ARTD_API_GPU_ENGINE Scene
{
    GpuEngine *owner_;
    friend class GpuEngineImpl;
    
    ObjectPtr<TransformNode> rootNode_;
    ObjectPtr<AnimationTaskList> animationTasks_;

protected:
    GpuEngineImpl *getOwner();
    typedef SceneNode super;
    
    void tickAnimations(TimingContext &timing);
    
    void removeActiveLight(LightNode *l);
    void addActiveLight(LightNode *l);

    // TODO: to be grouped by shader/pipeline  Just a hack for now.
    std::vector<MeshNode*> drawables_;

    void addDrawable(SceneNode *l);
    void removeDrawable(SceneNode *l);

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

    void onNodeAttached(SceneNode *n);
    void onNodeDetached(SceneNode *n);

    //    void addAnimationTask(SceneNode *owner, AnimationFunction f);
    void addAnimationTask(SceneNode *owner, ObjectPtr<AnimationTask> task);
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
