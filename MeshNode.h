#pragma once
#include "artd/TransformNode.h"


ARTD_BEGIN

class DrawableMesh;
struct InstanceData;
struct MaterialData;

class MeshNode
    : public TransformNode
{
    ObjectPtr<DrawableMesh> mesh_;
    ObjectPtr<MaterialData> material_; // TODO: needs better management for dynamism.
public:

    ~MeshNode();

    // load binding area for this node.  TODO: Virtual ? switch TBD
    void loadInstanceData(InstanceData &data);

    void setMesh(ObjectPtr<DrawableMesh> mesh);
    DrawableMesh *getMesh() const { return(mesh_.get()); }
    void setMaterial(ObjectPtr<MaterialData> newMat);

};


ARTD_END

