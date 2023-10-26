#include "artd/LightNode.h"

ARTD_BEGIN


namespace { // anonymous local namespace

 Matrix4f aimMatrixZAt(const glm::vec3 &direction) {

    glm::vec3 dir = glm::normalize(direction);

    Matrix4f ret;

    if(dir.y > -.999999f && dir.y < .999999f)  {

        Vec3f xVec = glm::cross(glm::vec3(0,1,0),dir);
        xVec = glm::normalize(xVec);

        Vec3f yVec = glm::cross(dir,xVec);
        yVec = glm::normalize(yVec);

        ret[0] = glm::vec4(xVec,0);
        ret[1] = glm::vec4(yVec,0);
        return(ret);
    } else {
        // TODO: though not usually used make sure is always
        // orthonormal and reasonable.  Used mainly for aiming lights.
        // in code

        // pointing down or up
        Vec3f yVec = glm::cross(dir,glm::vec3(1,0,0));
        yVec = glm::normalize(yVec);

        Vec3f xVec = glm::cross(yVec,dir);
        xVec = glm::normalize(xVec);

        ret[0] = glm::vec4(xVec,0);
        ret[1] = glm::vec4(yVec,0);
    }
    ret[2] = glm::vec4(dir,0);
    ret[3] = glm::vec4(0,0,0,1);
    return(ret);

}

} // ene namespace

LightNode::LightNode() {
    data_.orientation_ = glm::mat3(1.f);
    data_.position_ = glm::vec3(0,0,0);
    data_.type_ = Type::directional;
}
void
LightNode::setDirection(const glm::vec3 &direction) {

    Matrix4f lmat = aimMatrixZAt(direction);
    lmat[3] = getLocalTransform()[3];  // preserve position !
    setLocalTransform(lmat);
}

ARTD_END
