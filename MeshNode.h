#pragma once
#include "artd/TransformNode.h"


ARTD_BEGIN

class DrawableMesh;
struct InstanceData;

class MeshNode
    : public TransformNode
{
    ObjectPtr<DrawableMesh> mesh_;
public:

    ~MeshNode();

    // load binding area for this node.  TODO: Virtual ? switch TBD
    void loadInstanceData(InstanceData &data);

    void setMesh(ObjectPtr<DrawableMesh> mesh);
    DrawableMesh *getMesh() const { return(mesh_.get()); }
};


ARTD_END

