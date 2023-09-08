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
	wgpu::Device m_device;
	uint32_t m_width;
	uint32_t m_height;
	wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
	wgpu::RenderPassDescriptor m_renderPassDesc;
	wgpu::RenderPipeline m_pipeline = nullptr;
	wgpu::Texture m_renderTexture = nullptr;
	wgpu::TextureDescriptor m_renderTextureDesc;
	wgpu::TextureView m_renderTextureView = nullptr;
	wgpu::Buffer m_pixelBuffer = nullptr;
	wgpu::BufferDescriptor m_pixelBufferDesc;
};

ARTD_END
