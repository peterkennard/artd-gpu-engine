#pragma once
#include "artd/TransformNode.h"


ARTD_BEGIN

class DrawableMesh;
struct InstanceData;
class Material;

class MeshNode
    : public TransformNode
{
    ObjectPtr<DrawableMesh> mesh_;
    ObjectPtr<Material> material_; // TODO: needs better management for dynamism.
public:

    ~MeshNode();

    // load binding area for this node.  TODO: Virtual ? switch TBD
    void loadInstanceData(InstanceData &data);

    void setMesh(ObjectPtr<DrawableMesh> mesh);
    DrawableMesh *getMesh() const { return(mesh_.get()); }
    void setMaterial(ObjectPtr<Material> newMat);

};


ARTD_END

