#pragma once

#include "artd/gpu_engine.h"
#include <webgpu/webgpu.hpp>
#include "artd/Color3f.h"
#include "artd/ObjectBase.h"
#include "artd/IntrusiveList.h"

ARTD_BEGIN

class TextureView;

#define INL ARTD_ALWAYS_INLINE

struct MaterialShaderData  {
    glm::vec3 diffuse_;
    int32_t ix_;  // unused in shader
    glm::vec3 specular_;
    float pad0_;
};

static_assert(sizeof(MaterialShaderData) % 16 == 0);

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE Material
    : public DlNode  // for attaching to scene material list
{
public:
    Material(GpuEngineImpl *owner);
    ~Material();

    INL void loadShaderData(MaterialShaderData &data) {
        data = data_;
    }
    INL void setDiffuse(const Color3f &diffuse) {
        data_.diffuse_ = diffuse;
    }
    INL void setDiffuseTex(ObjectPtr<TextureView> tView) {
        diffuseTex_ = tView;
    }
    // only set by renderer.
    INL void setIndex(int index) {
        data_.ix_ = index;
    }
    INL int32_t getIndex() {
        return(data_.ix_);
    }
    INL wgpu::BindGroup &getBindings() {
        return(bindings_);
    }

    wgpu::BindGroup bindings_ = nullptr;
    ObjectPtr<TextureView> diffuseTex_;

    INL ObjectPtr<TextureView> &getDiffuseTexture() {
        return(diffuseTex_);
    }

private:
    MaterialShaderData data_;
};

#undef INL

ARTD_END
