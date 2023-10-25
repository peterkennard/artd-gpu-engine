#include "artd/LightNode.h"

ARTD_BEGIN


LightNode::LightNode() {
    data_.orientation_ = glm::mat3(1.f);
    data_.position_ = glm::vec3(0,0,0);
    data_.type_ = Type::directional;
}


ARTD_END
