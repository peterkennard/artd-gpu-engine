#include "artd/MeshNode.h"
#include "DrawableMesh.h"
#include "./GpuEngineImpl.h"

ARTD_BEGIN

MeshNode::MeshNode() {
    setFlags(fIsDrawable);
}

MeshNode::~MeshNode() {
}
void
MeshNode::setMaterial(ObjectPtr<Material> newMat) {
    material_ = newMat;
    if(newMat) {
        Scene *s = getScene();
        if(s) {
            s->addActiveMaterial(newMat.get());
        }
    }
}

void MeshNode::setMesh(ObjectPtr<DrawableMesh> mesh) {
    mesh_ = mesh;
}

void
MeshNode::loadInstanceData(struct InstanceData &data) {
    data.modelMatrix = getLocalToWorldTransform();
    data.materialId = 0;
    if(material_) {
        data.materialId = material_->getIndex();
    }
}


ARTD_END
