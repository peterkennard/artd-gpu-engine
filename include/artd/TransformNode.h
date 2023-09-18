#pragma once

#include "artd/TransformChild.h"
#include "artd/Matrix4f.h"
#include "artd/ObjectBase.h"

#define INL ARTD_ALWAYS_INLINE

ARTD_BEGIN

class ARTD_API_GPU_ENGINE TransformNode
    : public TransformChild
{
private:
    Matrix4f localTransform_; // transform to parent space
    Matrix4f modelToWorld_;  // cached build from tree updates when gotten
	// glm::mat3 normalMatrix_;  pose_ ? maybe put in drawable ?

    uint32_t localTransformModified_=3;
	uint32_t worldTransformModified_=3;

	INL bool worldTransformDirty() const {
		return((worldTransformModified_ & 0x01) != 0);
	}

protected:
    INL void setLocalTransformModified() {
        if ((localTransformModified_ & 0x01) == 0) {
            localTransformModified_ += 3;
        }
    }

    INL bool localTransformDirty() {
        return((localTransformModified_ & 0x01) != 0);
    }

    // test if lastCount is the currentCount.
    // if different set lastCount to the currentCount and return true
    // otherwise return false
    INL bool testSetLocalTransModified(int &lastCount) {
        if (lastCount != (int)localTransformModified_) {
            lastCount = localTransformModified_ & (~0x01);
            return(true);
        }
        return(false);
    }

   	INL void setWorldTransformModified() {
   		if ((worldTransformModified_ & 0x01) == 0) {
   			worldTransformModified_ += 3;
   		}
//           if (children_) {
//               for (Object * child : children_) {
//                   auto child = static_cast<Object*>(child);
//                   child->setWorldTransformModified();
//               }
//           }
   	}

    // TODO: should return reference ot value ?
	INL const Matrix4f getParentToWorldMatrix() {
        Matrix4f matParent;
		if (parent_) {
			matParent = parent_->getLocalToWorldTransform();
		}
		return(matParent);
	}

public:

    inline TransformNode() {
    }
    inline ~TransformNode() {
    }

    // test if lastCount is modified.
    // if modified set lastCount to the currentCount and return true
    // otherwise return false
	INL bool testSetWorldTransModified(int &lastCount) {
		if (lastCount != (int)worldTransformModified_) {
			lastCount = worldTransformModified_ & (~0x01);
			return(true);
		}
		return(false);
	}

	INL int getWorldTransformAlteredCount() {
		return(worldTransformModified_ & (~0x01));
	}

    INL const Matrix4f &getLocalTransform() {
        localTransformModified_ &= (~0x01);
        return(localTransform_);
    }
    INL void setLocalTransform(const glm::mat4 &lt) {
        localTransform_ = lt;
        setLocalTransformModified();
        setWorldTransformModified();
    }

    INL const Matrix4f &getLocalToWorldTransform() {
		if (worldTransformDirty() || localTransformDirty()) {
            worldTransformModified_ &= (~0x01);
			modelToWorld_ = getParentToWorldMatrix() * getLocalTransform();
		}
        return(modelToWorld_);
    }

    inline Matrix4f getWorldPose() {
        glm::mat4 rotation{};
        glm::vec3 scale{};
        glm::vec3 translation{};

        affineDecompose(getLocalToWorldTransform(), rotation, scale, translation);
        return(affineCompose(translation, rotation, glm::vec3(1, 1, 1)));
    }


};

#undef INL

ARTD_END
