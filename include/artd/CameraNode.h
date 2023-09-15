#pragma once

#include "artd/TransformNode.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class Camera;

class ARTD_API_GPU_ENGINE CameraNode
    : public TransformNode
{
    ObjectPtr<Camera> camera_;
public:

    void setCamera(ObjectPtr<Camera> cam);
    
    INL ObjectPtr<Camera> getCamera() {
        return(camera_);
    }
};

#undef INL

ARTD_END
