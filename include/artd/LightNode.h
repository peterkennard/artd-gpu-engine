#pragma once
#include "artd/TransformNode.h"
#include "artd/Color3f.h"

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE


// SEE: https://www.w3.org/TR/WGSL/#alignment-and-size
// for alignment specs  - this has to match the shader !!

class LightShaderData {
public:
    glm::mat3 orientation_;  // align 16 sizeof = 36 ( consumes 48 in buffer )
    float _pad1[3];
    // now at byte 48
    glm::vec3 position_;  // align 16 consumes 12 in buffer
    uint32_t type_; // align 4  - consumes 4
    // now at byte 64
    glm::vec4 diffuse_; // have the "alpha" or "w" be a "blacklight" component for "flourecense" ??
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
	        auto pose = getWorldPose();
            data_.orientation_ = glm::mat3(pose);
            data_.position_ = glm::vec3(pose[3]);
	    }
	    data = data_;
    }

    INL void setLightType(Type t) {
        data_.type_ = (uint32_t)t;
    }
    // TODO:
    INL void setDiffuse(const Color3f &color) {
        data_.diffuse_ = glm::vec4(color,0);
    }
};

#undef INL

ARTD_END
