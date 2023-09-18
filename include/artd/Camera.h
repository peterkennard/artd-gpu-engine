#pragma once

#include "artd/TransformChild.h"
#include "artd/Matrix4f.h"
#include "artd/Ray3f.h"
#include "artd/ObjectBase.h"
#include "artd/CameraNode.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class Viewport;
class CameraNode;

class ARTD_API_GPU_ENGINE Camera
    : public TransformChild
{
protected:

	ObjectPtr<Viewport> viewport_;

    float viewAngle_; // or field of view is in radians
    float nearClip_;
    float farClip_;

    int lastTransformStamp_ = -1;
    int lastViewportStamp_ = -1;

    Matrix4f view_;
    Matrix4f projection_;
    
    INL void setViewportAltered() {
        // note any even number (one bit 0) -3 will have the one bit on
        // the stamps are incremented by viewport and transform by adding
        // and the one bit returned by tresting the viewport or parent transform
        // is always 0.
        if((lastViewportStamp_ & 0x01) == 0) {
            lastViewportStamp_ -= 3;
        }
    }

public:

    void setParent(CameraNode *parent) {
        if((TransformNode*)parent != parent_) {
            parent_ = (TransformNode*)parent;
            lastTransformStamp_ = -1;
        }
    }

    INL void setViewport(ObjectPtr<Viewport> vp) {
        if(vp != viewport_) {
            viewport_ = vp;
            setViewportAltered();
        }
    }
    
    INL void setNearClip(float distance) {
        nearClip_ = distance;
        setViewportAltered();
    }
    
	INL void setFarClip(float distance) {
        farClip_ = distance;
        setViewportAltered();
    }

    INL void setFocalLength(float focalLength) {
        viewAngle_ = 2 * glm::atan(1.0 / focalLength);
        setViewportAltered();
    }
    const Matrix4f &getView();
	const Matrix4f &getProjection();

    static void poseToView(const glm::mat4 &pose, glm::mat4 &viewOut);
    static void viewToPose(const glm::mat4 &view, glm::mat4 &poseOut);

    glm::vec3 unProject(float mouseX, float mouseY, float mouseZ);

    Ray3f getPixelRay(int px, int py);
};


#undef INL

ARTD_END
