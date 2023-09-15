#include "artd/CameraNode.h"
#include "artd/Camera.h"

ARTD_BEGIN

void
CameraNode::setCamera(ObjectPtr<Camera> cam)
{
    camera_ = cam;
    cam->setParent(this);
}

ARTD_END
