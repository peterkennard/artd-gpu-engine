#include "artd/Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "artd/TransformNode.h"
#include "artd/Viewport.h"

ARTD_BEGIN

void
Camera::poseToView(const glm::mat4 &pose, glm::mat4 &viewOut) {

	if (&viewOut != &pose) {
		viewOut = pose;
	}
	// flip Z and X axes as that is what is needed fot he viewing transform worldToEye
	viewOut[0] = glm::vec4(-viewOut[0].x, -viewOut[0].y, -viewOut[0].z, 0);
	viewOut[2] = glm::vec4(-viewOut[2].x, -viewOut[2].y, -viewOut[2].z, 0);
	viewOut = glm::inverse(viewOut);
}

void
Camera::viewToPose(const glm::mat4 &view, glm::mat4 &poseOut) {
	poseOut = glm::inverse(view);
	// flip Z and X axes as that is what is needed for the pose eyeToWorld transform
	poseOut[0] = glm::vec4(-poseOut[0].x, -poseOut[0].y, -poseOut[0].z, 0);
	poseOut[2] = glm::vec4(-poseOut[2].x, -poseOut[2].y, -poseOut[2].z, 0);
}

#if 0
// TODO: we don't do this for now as we always inherit the pose
// from the parent node this is attached to.
// But could be useful for local cameras for things like
// separate render passes for shadows or picking PK
void
Camera::setView(const glm::mat4& view) {
    //	glm::mat4 pose;
    //	viewMatrixToPose(view, pose);
    //	setWorldModelMatrix(pose);
    //	getWorldModelMatrix();
}
#endif

const Matrix4f &
Camera::getView() {
    TransformNode *parent = getParent();
    if(parent != nullptr) {
	    if(parent->testSetWorldTransModified(lastTransformStamp_)) {
	        auto pose = parent->getWorldPose();
            poseToView(pose,view_);
	    }
    }
    return(view_);
}

const Matrix4f &
Camera::getProjection() {
	if(viewport_ && viewport_->testGetModifiedStamp(lastViewportStamp_)) {
        projection_ = glm::perspective(viewAngle_, viewport_->getAspectWtoH(), nearClip_, farClip_);
	}
	return(projection_);
}

glm::vec3
Camera::unProject(float mouseX, float mouseY, float mouseZ) {

	// project from viewport pixel co-ordinates to world coordinates
	// mouseZ should be 0 for near plane, and 1 for far plane
	float y = viewport_->getHeight() - mouseY - 1;
	return glm::unProject(glm::vec3(mouseX, y, mouseZ), view_, projection_, glm::vec4(0, 0, viewport_->getWidth(), viewport_->getHeight()));
}

Ray3f
Camera::getPixelRay(int pixelX, int pixelY) {

	glm::vec3 nearPoint = unProject((float)pixelX, (float)pixelY, 0.0f);
	glm::vec3 farPoint = unProject((float)pixelX, (float)pixelY, 1.0f);
	glm::vec3 dir = farPoint - nearPoint;
	return(Ray3f(nearPoint, glm::normalize(dir)));
}

ARTD_END

