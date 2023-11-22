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
    int32_t ix_;  // unused in shader for now
    glm::vec3 specular_;
    float shininess_;  // used for phong type specularity
    glm::vec4 emissive_;
};

static_assert(sizeof(MaterialShaderData) % 16 == 0);

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE Material
    : public DlNode  // for attaching to scene material list
{
    friend class GpuEngineImpl;

    INL void setIndex(int index) {
        data_.ix_ = index;
    }

public:
    Material(GpuEngineImpl *owner);
    ~Material();

    INL int32_t getIndex() {
        return(data_.ix_);
    }

    INL void loadShaderData(MaterialShaderData &data) {
        data = data_;
    }

    INL void setDiffuse(const Color3f &diffuse) {
        data_.diffuse_ = diffuse;
    }

    INL void setEmissive(const Color3f &emissive) {
        data_.emissive_ = glm::vec4(emissive.r, emissive.g, emissive.b, 1.0);
    }

    INL void setShininess(float shininess) {
        data_.shininess_ = shininess;
    }
    INL void setDiffuseTex(ObjectPtr<TextureView> tView) {
        diffuseTex_ = tView;
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
