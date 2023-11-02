#pragma once

#include "artd/gpu_engine.h"
#include <webgpu/webgpu.hpp>

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

class GpuEngineImpl;

class ARTD_API_GPU_ENGINE PickerPass {
    GpuEngineImpl &owner_;
public:
    INL GpuEngineImpl &getOwner() {
        return(owner_);
    }
    PickerPass(GpuEngineImpl *owner);
    ~PickerPass();
private:
    wgpu::Device device_;
    uint32_t width_;
    uint32_t height_;
    wgpu::BindGroupLayout bindGroupLayout_ = nullptr;
    wgpu::RenderPassDescriptor renderPassDesc_;
    wgpu::RenderPipeline pipeline_ = nullptr;
    wgpu::Texture renderTexture_ = nullptr;
    wgpu::TextureDescriptor renderTextureDesc_;
    wgpu::TextureView renderTextureView_ = nullptr;
};

#undef INL

ARTD_END

