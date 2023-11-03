#include "GpuEngineImpl.h"
#include "./PickerPass.h"

struct GLFWwindow;

ARTD_BEGIN

#define INL ARTD_ALWAYS_INLINE

PickerPass::PickerPass(GpuEngineImpl *owner)
    : owner_(*owner), device_(owner->device)
{
	using namespace wgpu;

    // todo: pick best size.
    width_ = 0x40;  // 64
    height_ = 0x40;

	// Create a texture onto which we blit the texture view
	TextureDescriptor renderTextureDesc;
	renderTextureDesc.dimension = TextureDimension::_2D;
	renderTextureDesc.format = TextureFormat::RGBA8Unorm;
	renderTextureDesc.mipLevelCount = 1;
	renderTextureDesc.sampleCount = 1;
	renderTextureDesc.size = { width_, height_, 1 };
    renderTextureDesc.usage = TextureUsage::RenderAttachment | TextureUsage::TextureBinding | TextureUsage::CopyDst; // TextureUsage::CopySrc;
	renderTextureDesc.viewFormatCount = 0;
	renderTextureDesc.viewFormats = nullptr;
	renderTexture_ = device_.createTexture(renderTextureDesc);

	TextureViewDescriptor renderTextureViewDesc;
	renderTextureViewDesc.aspect = TextureAspect::All;
	renderTextureViewDesc.baseArrayLayer = 0;
	renderTextureViewDesc.arrayLayerCount = 1;
	renderTextureViewDesc.baseMipLevel = 0;
	renderTextureViewDesc.mipLevelCount = 1;
	renderTextureViewDesc.dimension = TextureViewDimension::_2D;
	renderTextureViewDesc.format = renderTextureDesc.format;
	renderTextureView_ = renderTexture_.createView(renderTextureViewDesc);
    
    
    // Create test image data -
    std::vector<uint8_t> pixels(4 * width_ * height_);
    for (uint32_t i = 0; i < width_; ++i) {
        for (uint32_t j = 0; j < height_; ++j) {
            uint8_t *p = &pixels[4 * (j * width_ + i)];
            p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
            p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
            p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
            p[3] = 255; // a
        }
    }
    // copy pixels into texture
    
    // Upload texture data
    // Arguments telling which part of the texture we upload to
    // (together with the last argument of writeTexture)
    ImageCopyTexture destination;
    destination.texture = renderTexture_;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = TextureAspect::All; // only relevant for depth/Stencil textures
    
    // Arguments telling how the C++ side pixel memory is laid out
    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = 4 * renderTextureDesc.size.width;
    source.rowsPerImage = renderTextureDesc.size.height;

    // Issue commands
    Queue queue = device_.getQueue();
    queue.writeTexture(destination, pixels.data(), pixels.size(), source, renderTextureDesc.size);
    
}
PickerPass::~PickerPass() {
    renderTexture_.destroy();
    renderTexture_.release();
}

ARTD_END
