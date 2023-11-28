#pragma once

#include "artd/gpu_engine.h"
#include "artd/Texture.h"
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

    INL ObjectPtr<TextureView>  &getTextureView() {
        return(texView_);
    }

private:
    wgpu::Device device_;
    uint32_t width_;
    uint32_t height_;
    
    ObjectPtr<TextureView> texView_;
    
    wgpu::BindGroupLayout bindGroupLayout_ = nullptr;
    wgpu::RenderPassDescriptor renderPassDesc_;
    wgpu::RenderPipeline pipeline_ = nullptr;
    wgpu::TextureDescriptor renderTextureDesc_;
};

#undef INL

ARTD_END

