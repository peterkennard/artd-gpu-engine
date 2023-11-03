#include "MeshNode.h"
#include "DrawableMesh.h"
#include "./GpuEngineImpl.h"

ARTD_BEGIN

MeshNode::~MeshNode() {

}
void
MeshNode::setMaterial(ObjectPtr<Material> newMat) {
    material_ = newMat;
}

void MeshNode::setMesh(ObjectPtr<DrawableMesh> mesh) {
    mesh_ = mesh;
}

void
MeshNode::loadInstanceData(struct InstanceData &data) {
    data.modelMatrix = getLocalToWorldTransform();
    data.materialId = 0;
    if(material_) {
        data.materialId = material_->getId();
    }
}


ARTD_END
