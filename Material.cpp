#include "./GpuEngineImpl.h"
#include "./TextureManager.h"

ARTD_BEGIN


Material::Material(GpuEngine *owner) {
    std::memset(&data_,0,sizeof(data_));
    GpuEngineImpl *e = static_cast<GpuEngineImpl*>(owner);
    bindings_ = e->defaultMaterialBindGroup_;
}

Material::~Material() {
    GpuEngineImpl *e = &GpuEngineImpl::getInstance();
    if(bindings_ &&
       (void *)bindings_ != (void*)(e->defaultMaterialBindGroup_))
    {
        bindings_.release();
    }
}

void
Material::setDiffuseTexture(StringArg resPath) {
    GpuEngineImpl *e = &GpuEngineImpl::getInstance();
    Material *pMat = this;
    
    // TODO: need to get a referenced pointer to this if async;
    
    e->textureManager_->loadBindableTexture(resPath, [pMat,e](ObjectPtr<TextureView> tView) {
            pMat->setDiffuseTex(tView);
            pMat->bindings_ = e->createMaterialBindGroup(pMat);
        });
}

ARTD_END

