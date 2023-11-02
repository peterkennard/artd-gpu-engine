#pragma once

#include <webgpu/webgpu.hpp>

#include <filesystem>
#include <string>
#include "artd/jlib_base.h"

ARTD_BEGIN

class PixelReader {
public:
	PixelReader(wgpu::Device device, uint32_t width, uint32_t height);
	// bool render(const std::filesystem::path path, wgpu::TextureView textureView) const;
    bool lockPixels(uint32_t **pPixelsOut, wgpu::Texture texture) const;
    void unlockPixels();

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
	wgpu::Buffer pixelBuffer_ = nullptr;
	wgpu::BufferDescriptor pixelBufferDesc_;
};

ARTD_END
