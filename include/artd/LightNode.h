#pragma once
#include "artd/TransformNode.h"
#include "artd/Color3f.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE


// SEE: https://www.w3.org/TR/WGSL/#alignment-and-size
// for alignment specs  - this has to match the shader !!

class LightShaderData {
public:
    glm::mat4 pose_;
    glm::vec3 diffuse_; // have the "alpha" or "w" be a "blacklight" component for "flourecense" ??
    uint32_t type_; // align 4  - consumes 4
    // now at byte 80

//    glm::vec3 ambient_;  // should only an ambient light have this value or should only be diffuse as ambient would just be specular everywhere ?
//    glm::vec3 specular_;
//    glm::vec3 falloff_; // linear, constant, quadratic
//
//    // spot light settings
//    float spotAngle_; // 1.5;
//    float spotBlur_;  // 0 == sharp edges and uniform brightness,
//                      // 1.0 results in a cosine curve of brightness
//
//    float cosAngle_2; // precalculated for shader ~cos(spotAngle_/2.0), mirrors spotAngle_

};

// Have the compiler check byte alignment
static_assert(sizeof(LightShaderData) % 16 == 0);


class ARTD_API_GPU_ENGINE LightNode
    : public TransformNode
{
protected:


    int lastTransformStamp_ = -1;
    LightShaderData data_;

public:

	enum Type {
		directional,
		ambient,
		point,
		spot,
		NONE
	};

    LightNode();

    INL void loadShaderData(LightShaderData &data) {
	    if(testSetWorldTransModified(lastTransformStamp_)) {
            data_.pose_ = getWorldPose();
	    }
	    data = data_;
    }

    INL void setLightType(Type t) {
        data_.type_ = (uint32_t)t;
    }
    
    // will set direction (orientation) of node such that it's Z is the direction
    // and the orientation's Y is "up" unless the direction is looking near up or down.
    // this will be relative to the parent node !
    // TODO: probably shoudl be a transform node method "setZFacing" or whatever, like facing the camera or some
    // other object.
    
    void setDirection(const glm::vec3 &direction);
    
    // TODO:
    INL void setDiffuse(const Color3f &color) {
        data_.diffuse_ = glm::vec4(color,0);
    }
};

#undef INL

ARTD_END
