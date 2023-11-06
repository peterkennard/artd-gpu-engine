#pragma once

#include "artd/gpu_engine.h"
#include <webgpu/webgpu.hpp>
#include "artd/Color3f.h"
#include "artd/ObjectBase.h"

ARTD_BEGIN

class TextureView;

#define INL ARTD_ALWAYS_INLINE

struct MaterialShaderData  {
    glm::vec3 diffuse_;
    int32_t id_;
    glm::vec3 specular_;
    float pad0_;
};

static_assert(sizeof(MaterialShaderData) % 16 == 0);

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE Material {
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
    INL void setId(int id) {
        data_.id_ = id;
    }
    INL int32_t getId() {
        return(data_.id_);
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
