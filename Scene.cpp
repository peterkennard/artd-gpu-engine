#include "./GpuEngineImpl.h"
#include "artd/Scene.h"

ARTD_BEGIN

Scene::Scene(GpuEngine *e)
    : owner_(e)
{
    setId(0);
    clearFlags(fHasTransform | fHasParent);
    parent_ = nullptr;
}

Scene::~Scene()
{
}

SceneNode *
Scene::addSceneChild(ObjectPtr<SceneNode> child) {
    AD_LOG(info) << ( void *)(child.get());
    return(nullptr);
}


ARTD_END


