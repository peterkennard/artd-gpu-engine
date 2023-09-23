#include "MeshNode.h"
#include "DrawableMesh.h"

ARTD_BEGIN

MeshNode::~MeshNode() {

}

void
MeshNode::setMesh(ObjectPtr<DrawableMesh> mesh) {
    mesh_ = mesh;
}

ARTD_END
