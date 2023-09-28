#include "MeshNode.h"
#include "DrawableMesh.h"
#include "./GpuEngineImpl.h"

ARTD_BEGIN

MeshNode::~MeshNode() {

}

void
MeshNode::loadInstanceData(struct InstanceData &data) {
    data.modelMatrix = getLocalToWorldTransform();
}

void MeshNode::setMesh(ObjectPtr<DrawableMesh> mesh) {
    mesh_ = mesh;
}

ARTD_END
