#include "./GpuEngineImpl.h"
#include "./TextureManager.h"

ARTD_BEGIN


Material::Material(GpuEngine */*owner*/) {
    std::memset(&data_,0,sizeof(data_));
}

Material::~Material() {

}

void
Material::setDiffuseTexture(StringArg resPath) {
    GpuEngineImpl *e = &GpuEngineImpl::getInstance();
    Material *pMat = this;
    
    // TODO: need to get a referenced poionter to this is async;
    
    e->textureManager_->loadBindableTexture(resPath, [pMat,e](ObjectPtr<TextureView> tView) {
            pMat->setDiffuseTex(tView);
            pMat->bindings_ = e->createMaterialBindGroup(*pMat);
        });
}

ARTD_END

