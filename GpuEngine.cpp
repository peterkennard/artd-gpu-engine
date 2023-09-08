/**
 * This file is originally base on  the  "Learn WebGPU for C++" book.
 *   https://github.com/eliemichel/LearnWebGPU
 *   'tis a major fork and redone a lot.
 * 
 * MIT License
 * Copyright (c) 2022-2023 Elie Michel
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <glfw3webgpu.h>

#define WEBGPU_CPP_IMPLEMENTATION  // declares method implementations
#include <webgpu/webgpu.hpp>
#undef WEBGPU_CPP_IMPLEMENTATION  // declares method implementations

#include "GpuEngineImpl.h"

#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include "artd/GpuEngine-PanamaExports.h"


ARTD_BEGIN

using namespace wgpu;


GpuEngine::GpuEngine() {

};
GpuEngine::~GpuEngine() {

};


namespace fs = std::filesystem;


/**
 * The same structure as in the shader, replicated in C++
 */

//
//struct MyUniforms {
//	// offset = 0 * sizeof(vec4f) -> OK
//	std::array<float, 4> color;
//
//	// offset = 16 = 4 * sizeof(f32) -> OK
//	float time;
//
//	// Add padding to make sure the struct is host-shareable
//	float _pad[3];
//};
// Have the compiler check byte alignment
//static_assert(sizeof(MyUniforms) % 16 == 0);

    
int GpuEngineImpl::init(bool headless, int width, int height) {
    width_ = width;
    height_ = height;
    headless_ = headless;

    AD_LOG(info) << "initializing " << (headless_ ? "headless" : "windowed") << " test engine";

    InstanceDescriptor instanceDescriptor = {};

    std::vector<const char*> enabledToggles = {
        "allow_unsafe_apis",
    };
    DawnTogglesDescriptor dawnToggles;
    dawnToggles.chain.next = nullptr;
    dawnToggles.chain.sType = SType::DawnTogglesDescriptor;
    dawnToggles.enabledTogglesCount = enabledToggles.size();
    dawnToggles.enabledToggles = enabledToggles.data();
    dawnToggles.disabledTogglesCount = 0;

    instanceDescriptor.nextInChain = &dawnToggles.chain;


    instance = createInstance(instanceDescriptor);
    if (!instance) {
        AD_LOG(error) << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    if(!headless_) {
        if (!glfwInit()) {
            std::cerr << "Could not initialize GLFW!" << std::endl;
            return 1;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width_, height_, "Learn WebGPU", NULL, NULL);
        if (!window) {
            std::cerr << "Could not open window!" << std::endl;
            return 1;
        }
        surface = glfwGetWGPUSurface(instance, window);
    } else {
        surface = nullptr;  // done in constructor
    }

    AD_LOG(info) << "Requesting adapter...";

    RequestAdapterOptions adapterOpts;
    adapterOpts.compatibleSurface = surface;
    //                              ^^^^^^^ This was 'surface'
    adapter = instance.requestAdapter(adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;
    DeviceDescriptor deviceDesc;
    deviceDesc.label = "My Device";
    deviceDesc.requiredFeaturesCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    device = adapter.requestDevice(deviceDesc);
    std::cout << "Got device: " << device << std::endl;

    // Add an error callback for more debug info
    errorCallback_ = device.setUncapturedErrorCallback([](ErrorType type, char const* message) {
        AD_LOG(error) << "Device error: type " << type << "(" << (message ? message : "" ) << ")";
    });

    queue = device.getQueue();

    TextureFormat swapChainFormat = TextureFormat::BGRA8Unorm;

    if(headless_) {
        // No more swap chain, but still a target format and a target texture

        TextureDescriptor targetTextureDesc;
        targetTextureDesc.label = "Render texture";
        targetTextureDesc.dimension = TextureDimension::_2D;
        // Any size works here, this is the equivalent of the window size
        targetTextureDesc.size = { width_, height_, 1 };
        // Use the same format here and in the render pipeline's color target
        targetTextureDesc.format = swapChainFormat;
        // No need for MIP maps
        targetTextureDesc.mipLevelCount = 1;
        // You may set up supersampling here
        targetTextureDesc.sampleCount = 1;
        // At least RenderAttachment usage is needed. Also add CopySrc to be able
        // to retrieve the texture afterwards.
        targetTextureDesc.usage = TextureUsage::RenderAttachment | TextureUsage::CopySrc;
        targetTextureDesc.viewFormats = nullptr;
        targetTextureDesc.viewFormatCount = 0;
        targetTexture = device.createTexture(targetTextureDesc);

        TextureViewDescriptor targetTextureViewDesc;
        targetTextureViewDesc.label = "Render texture view";
        // Render to a single layer
        targetTextureViewDesc.baseArrayLayer = 0;
        targetTextureViewDesc.arrayLayerCount = 1;
        // Render to a single mip level
        targetTextureViewDesc.baseMipLevel = 0;
        targetTextureViewDesc.mipLevelCount = 1;
        // Render to all channels
        targetTextureViewDesc.aspect = TextureAspect::All;
        targetTextureView = targetTexture.createView(targetTextureViewDesc);

    } else {

        AD_LOG(info) << "Creating swapchain...";
    #ifdef WEBGPU_BACKEND_WGPU
        swapChainFormat = surface.getPreferredFormat(adapter);
    #endif
        SwapChainDescriptor swapChainDesc;
        swapChainDesc.width = width_;
        swapChainDesc.height = height_;
        swapChainDesc.usage = TextureUsage::RenderAttachment;
        swapChainDesc.format = swapChainFormat;
        swapChainDesc.presentMode = PresentMode::Fifo;
        swapChain = device.createSwapChain(surface, swapChainDesc);
        AD_LOG(info) << "Swapchain created " << swapChain;
    }

    std::cout << "Creating shader module..." << std::endl;
    const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
var p = vec2f(0.0, 0.0);
if (in_vertex_index == 0u) {
    p = vec2f(-0.5, -0.5);
} else if (in_vertex_index == 1u) {
    p = vec2f(0.5, -0.5);
} else {
    p = vec2f(0.0, 0.5);
}
return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

    ShaderModuleDescriptor shaderDesc;
#ifdef WEBGPU_BACKEND_WGPU
    shaderDesc.hintCount = 0;
    shaderDesc.hints = nullptr;
#endif

    // Use the extension mechanism to load a WGSL shader source code
    ShaderModuleWGSLDescriptor shaderCodeDesc;
    // Set the chained struct's header
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    // Setup the actual payload of the shader code descriptor
    shaderCodeDesc.code = shaderSource;

    shaderModule = device.createShaderModule(shaderDesc);
    AD_LOG(info) << "Shader module: " << shaderModule;

    AD_LOG(info) << "Creating render pipeline...";
    RenderPipelineDescriptor pipelineDesc;

    // Vertex fetch
    // (We don't use any input buffer so far)
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = nullptr;

    // Vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    // Primitive assembly and rasterization
    // Each sequence of 3 vertices is considered as a triangle
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    pipelineDesc.primitive.cullMode = CullMode::None;

    // Fragment shader
    FragmentState fragmentState;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    // Configure blend state
    BlendState blendState;
    // Usual alpha blending for the color:
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    // We leave the target alpha untouched:
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTarget;
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    // Depth and stencil tests are not used here
    pipelineDesc.depthStencil = nullptr;

    // Multi-sampling
    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    // Pipeline layout
    pipelineDesc.layout = nullptr;

    pipeline = device.createRenderPipeline(pipelineDesc);
    AD_LOG(info) << "Render pipeline: " << pipeline;

    if(headless_) {
        pixelGetter_ = artd::ObjectBase::make<PixelReader>(device, width_,height_);
        pixelUnLockLock_.signal(); // start enabled !!
    }
    return(0);
}

void
GpuEngineImpl::presentImage(TextureView /*texture*/ )  {
    if(headless_) {
        pixelLockLock_.signal();
    } else {
        swapChain.present();
    }
}
TextureView
GpuEngineImpl::getNextTexture() {
    if(headless_) {
        return(targetTextureView);
    } else {
        return(swapChain.getCurrentTextureView());
    }
}

int
GpuEngineImpl::getPixels(uint32_t *pBuf) {
    return(pixelGetter_->lockPixels(&pBuf, targetTexture));
}

const int *
GpuEngineImpl::lockPixels(int timeoutMillis) {
    uint32_t * pBuf = nullptr;
    if(pixelLockLock_.waitOnSignal(timeoutMillis) == 0) {
        // we got the lock
        pixelGetter_->lockPixels(&pBuf, targetTexture);
        // AD_LOG(error) << "pixels locked ######### " << (void *)pBuf;
        return((const int *)pBuf);
    }
    return((const int *)pBuf);
}

void
GpuEngineImpl::unlockPixels()  {
    pixelUnLockLock_.signal();
    pixelGetter_->unlockPixels();
}


int
GpuEngineImpl::renderFrame()  {

    if(headless_) {

        int ret = pixelUnLockLock_.waitOnSignal(1000);
        if(ret != 0) {
            AD_LOG(error) << " signal error " << ret;
        }

    } else {
        glfwPollEvents();
        if(glfwWindowShouldClose(window)) {
            return(1);
        }
    }
    if(!instance) {
        return(-1);
    }
    TextureView nextTexture = getNextTexture();
    if (!nextTexture) {
        AD_LOG(error) << "Cannot acquire next swap chain texture";
        return(1);
    }

    CommandEncoderDescriptor commandEncoderDesc;
    commandEncoderDesc.label = "Command Encoder";
    CommandEncoder encoder = device.createCommandEncoder(commandEncoderDesc);

    RenderPassDescriptor renderPassDesc;

    RenderPassColorAttachment renderPassColorAttachment;
    renderPassColorAttachment.view = nextTexture;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{ 0.9, 0.1, 0.2, 1.0 };
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;

    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWriteCount = 0;
    renderPassDesc.timestampWrites = nullptr;
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

    // In its overall outline, drawing a triangle is as simple as this:
    // Select which render pipeline to use
    renderPass.setPipeline(pipeline);
    // Draw 1 instance of a 3-vertices shape
    renderPass.draw(3, 1, 0, 0);

    renderPass.end();

    if(!headless_) {
        nextTexture.release();
    }
    CommandBufferDescriptor cmdBufferDescriptor;
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(command);

    // device.tick();
    // Instead of swapChain.present()
    // saveTexture("output.png", device, targetTexture);

    //saveTextureView("output.png", device, nextTexture, targetTexture.getWidth(), targetTexture.getHeight());
    presentImage(nextTexture);
    return(0);
}

void
GpuEngineImpl::releaseResources() {
    if(instance) {
        device.release();
        adapter.release();
        instance.release();
        instance = nullptr;
    }
    if(headless_) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

ARTD_END

#ifndef _WIN32
    __attribute__((constructor))
    static void initializer(void) {
        AD_LOG(info) << "loaded into process";
    }

    // Finalizer.
    __attribute__((destructor))
    static void finalizer(void) {
        AD_LOG(info) << "unloaded from process";
    }
#endif



extern "C" {

    typedef artd::GpuEngineImpl Engine;

    ARTD_API_GPU_ENGINE int initGPUTest(int width, int height) {
        AD_LOG(print) << "\n\n\n";
        return(Engine::getInstance().init(true, width,height));
    }

    ARTD_API_GPU_ENGINE int renderGPUTestFrame()  {
        return(Engine::getInstance().renderFrame());
    }

    ARTD_API_GPU_ENGINE int getPixels(int *pBuf) {
        return(Engine::getInstance().getPixels((uint32_t*)pBuf));
    }

    ARTD_API_GPU_ENGINE const int *lockPixels(int timeoutMillis) {
        return(Engine::getInstance().lockPixels(timeoutMillis));
    }

    ARTD_API_GPU_ENGINE void unlockPixels() {
        return(Engine::getInstance().unlockPixels());
    }

    ARTD_API_GPU_ENGINE void shutdownGPUTest()  {
        AD_LOG(info) << "shutting down WebGPU engine";
        Engine::getInstance().releaseResources();
    }

    ARTD_API_GPU_ENGINE int runGpuTest(int, char **) {
        Engine &test = Engine::getInstance();
        
        int ret = test.init(false, 1920, 1080);
        if(ret != 0) {
            return(ret);
        }
        
        for(;;) {
            ret = test.renderFrame();
            if(ret != 0) {
                break;
            }
        }
        test.releaseResources();
        return(ret);
    }

}

