#pragma once
#include "artd/TransformNode.h"

ARTD_BEGIN

class LightNode
    : public TransformNode
{
protected:

	enum Type {
		ambient,
		point,
		directional,
		spot,
		NONE
	};

	class LightData {
    public:
		glm::mat4 pose_;  // maybe should be the world transform rotational part maintained that way a mat3 ?
		glm::vec3 ambient_;  // should only an ambient light have this value or should only be diffuse as ambient would just be specular everywhere ?
		glm::vec3 diffuse_;
		glm::vec3 specular_;
		glm::vec3 falloff_; // linear, constant, quadratic

		// spot light settings
		float spotAngle_; // 1.5;
		float spotBlur_;  // 0 == sharp edges and uniform brightness,
						  // 1.0 results in a cosine curve of brightness

		float cosAngle_2; // precalculated for shader ~cos(spotAngle_/2.0), mirrors spotAngle_


		int32_t type_ = Type::spot; // part of flags ? maybe
		int32_t on_ = true; // incorporate with other flags ?
    };
};


ARTD_END