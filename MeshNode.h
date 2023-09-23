#pragma once
#include "artd/TransformNode.h"


ARTD_BEGIN

class DrawableMesh;

class MeshNode
    : public TransformNode
{
    ObjectPtr<DrawableMesh> mesh_;
public:

    ~MeshNode();

    void setMesh(ObjectPtr<DrawableMesh> mesh);
    DrawableMesh *getMesh() const { return(mesh_.get()); }
};


ARTD_END

